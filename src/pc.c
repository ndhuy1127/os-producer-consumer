#define _POSIX_C_SOURCE 200809L
#include "pc.h"
#include "buffer.h"

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct pc_context {
    struct pc_config config; /* Immutable while workers run. */
    size_t total_items;      /* P*K, checked before multiplication. */
    struct buffer buffer;
    sem_t empty, full, guard;
    int empty_initialized, full_initialized, guard_initialized;
    uint64_t *slot_sequence;
    unsigned int *produced, *consumed;
    uint64_t enqueues, dequeues;
    size_t record_capacity;
    struct pc_result *result;
};

struct worker_args {
    struct pc_context *context;
    size_t index;     /* Producer p or consumer c. */
    size_t completed; /* Written by this worker; main reads only after join. */
};

static const char *buffer_error(int status)
{
    switch (status) {
    case BUFFER_OK: return "BUFFER_OK";
    case BUFFER_INVALID_ARGUMENT: return "BUFFER_INVALID_ARGUMENT";
    case BUFFER_FULL: return "BUFFER_FULL";
    case BUFFER_EMPTY: return "BUFFER_EMPTY";
    case BUFFER_SIZE_OVERFLOW: return "BUFFER_SIZE_OVERFLOW";
    case BUFFER_NO_MEMORY: return "BUFFER_NO_MEMORY";
    case BUFFER_ALREADY_INITIALIZED: return "BUFFER_ALREADY_INITIALIZED";
    default: return "unknown buffer status";
    }
}

static _Noreturn void fatal(const char *operation, int error)
{
    fprintf(stderr, "fatal: %s: %s; terminating process (workers may be live)\n",
            operation, strerror(error));
    /* Do not run cleanup/atexit handlers against context still used by workers. */
    fflush(stderr);
    _Exit(PC_RUNTIME_ERROR);
}

static _Noreturn void fatal_buffer(const char *operation, int error)
{
    fprintf(stderr, "fatal: %s: %s (%d); terminating process\n",
            operation, buffer_error(error), error);
    fflush(stderr);
    _Exit(PC_RUNTIME_ERROR);
}

static int wait_retry(sem_t *semaphore)
{
    while (sem_wait(semaphore) == -1) {
        if (errno != EINTR) {
            return errno;
        }
    }
    return 0;
}

static enum pc_wait_reason take_permission(sem_t *semaphore,
                                           enum pc_wait_reason reason)
{
    for (;;) {
        if (sem_trywait(semaphore) == 0) {
            return PC_WAIT_NONE; /* Already owns a token: do not wait again. */
        }
        int error = errno;
        if (error == EINTR) {
            continue;
        }
        if (error != EAGAIN) {
            fatal("sem_trywait permission", error);
        }
        error = wait_retry(semaphore);
        if (error != 0) {
            fatal("sem_wait permission", error);
        }
        return reason; /* EAGAIN observed, not proof of kernel blocking/count. */
    }
}

static void post_checked(sem_t *semaphore, const char *operation)
{
    if (sem_post(semaphore) == -1) {
        fatal(operation, errno);
    }
}

static void take_guard(struct pc_context *context, sem_t *reserved)
{
    int error = wait_retry(&context->guard);
    if (error != 0) {
        /* No buffer change yet: return the token held for empty/full. */
        if (reserved != NULL) {
            post_checked(reserved, "sem_post restore reserved permission");
        }
        fatal("sem_wait guard", error);
    }
}

static void sleep_ms(unsigned int milliseconds)
{
    if (milliseconds == 0) {
        return;
    }
    struct timespec remaining = {
        .tv_sec = (time_t)(milliseconds / 1000U),
        .tv_nsec = (long)(milliseconds % 1000U) * 1000000L
    };
    while (nanosleep(&remaining, &remaining) == -1) {
        if (errno != EINTR) {
            fatal("nanosleep", errno);
        }
    }
}

/* All record/oracle/buffer access below occurs with guard held. */
static void record_operation(struct pc_context *context, enum pc_operation op,
                             enum pc_actor actor, size_t actor_index,
                             struct item item, uint64_t sequence,
                             enum pc_wait_reason reason)
{
    struct pc_result *result = context->result;
    struct pc_summary *summary = &result->summary;
    if (result->record_count >= context->record_capacity) {
        fatal("operation record capacity", EOVERFLOW);
    }
    size_t count = context->buffer.count;
    result->records[result->record_count] = (struct pc_record){
        .op_seq = (uint64_t)result->record_count + 1,
        .op = op, .actor = actor, .item = item, .enqueue_seq = sequence,
        .count_after = count, .capacity = context->config.capacity,
        .wait_reason = reason, .actor_index = actor_index
    };
    ++result->record_count;
    if (count > context->config.capacity ||
        context->buffer.head >= context->config.capacity ||
        context->buffer.tail >= context->config.capacity) {
        ++summary->count_errors;
    }
    if (count < summary->min_count) summary->min_count = count;
    if (count > summary->max_count) summary->max_count = count;
    if (reason == PC_WAIT_SLOT) ++summary->wait_slot;
    if (reason == PC_WAIT_ITEM) ++summary->wait_item;
    if (op == PC_DEQUEUE && sequence != context->dequeues) {
        ++summary->fifo_errors;
    }
    if (item.kind == ITEM_STOP) {
        if (op == PC_ENQUEUE) {
            ++summary->stop_enqueued;
        } else {
            ++summary->stop_dequeued;
            if (actor_index < PC_MAX_WORKERS) ++summary->consumer_stops[actor_index];
        }
        return;
    }
    if (op == PC_ENQUEUE) ++summary->data_enqueued;
    else ++summary->data_dequeued;
    if (item.id >= context->total_items) {
        ++summary->out_of_range;
        return;
    }
    if (item.value != (int)(item.id % 1000U) + 1) {
        ++summary->value_errors;
    }
    unsigned int *frequency = op == PC_ENQUEUE ? context->produced : context->consumed;
    ++frequency[(size_t)item.id];
}

static void enqueue(struct pc_context *context, struct item item,
                    enum pc_actor actor, size_t actor_index)
{
    enum pc_wait_reason reason = take_permission(&context->empty, PC_WAIT_SLOT);
    take_guard(context, &context->empty);
    size_t slot = context->buffer.tail;
    int error = buffer_push(&context->buffer, item);
    if (error != BUFFER_OK) {
        post_checked(&context->guard, "sem_post guard after failed push");
        post_checked(&context->empty, "sem_post empty after failed push");
        fatal_buffer("buffer_push", error);
    }
    context->slot_sequence[slot] = ++context->enqueues;
    record_operation(context, PC_ENQUEUE, actor, actor_index, item,
                     context->enqueues, reason);
    post_checked(&context->guard, "sem_post guard after enqueue");
    /* Push succeeded: never restore empty as though it had not happened. */
    post_checked(&context->full, "sem_post full after enqueue");
}

static struct item dequeue(struct pc_context *context, size_t consumer_index)
{
    enum pc_wait_reason reason = take_permission(&context->full, PC_WAIT_ITEM);
    take_guard(context, &context->full);
    uint64_t sequence = context->slot_sequence[context->buffer.head];
    struct item item; /* Copy independent of buffer storage. */
    int error = buffer_pop(&context->buffer, &item);
    if (error != BUFFER_OK) {
        post_checked(&context->guard, "sem_post guard after failed pop");
        post_checked(&context->full, "sem_post full after failed pop");
        fatal_buffer("buffer_pop", error);
    }
    ++context->dequeues;
    record_operation(context, PC_DEQUEUE, PC_CONSUMER, consumer_index, item,
                     sequence, reason);
    post_checked(&context->guard, "sem_post guard after dequeue");
    post_checked(&context->empty, "sem_post empty after dequeue"); /* Also STOP. */
    return item;
}

static void *producer(void *argument)
{
    struct worker_args *args = argument;
    const struct pc_config *config = &args->context->config;
    /* Producer p owns IDs p*K..p*K+K-1, so no two producers share an ID. */
    uint64_t first = (uint64_t)args->index * config->items_per_producer;
    for (size_t j = 0; j < config->items_per_producer; ++j) {
        uint64_t id = first + j;
        struct item item = {ITEM_DATA, id, (int)(id % 1000U) + 1};
        enqueue(args->context, item, PC_PRODUCER, args->index);
        ++args->completed;
        sleep_ms(config->producer_delay_ms);
    }
    return NULL;
}

static void *consumer(void *argument)
{
    struct worker_args *args = argument;
    for (;;) {
        struct item item = dequeue(args->context, args->index);
        if (item.kind == ITEM_STOP) {
            return NULL; /* Each consumer takes exactly one STOP. */
        }
        /* Simulated DATA processing and result belong to this worker. */
        ++args->completed;
        sleep_ms(args->context->config.consumer_delay_ms);
    }
}

struct pc_config pc_default_config(void)
{
    return (struct pc_config){8, 1, 1, 20, 0, 0};
}

static int config_valid(const struct pc_config *config)
{
    if (config == NULL || config->capacity == 0 ||
        config->capacity > PC_MAX_CAPACITY ||
        config->capacity > SEM_VALUE_MAX || config->capacity > UINT_MAX ||
        config->producers == 0 || config->producers > PC_MAX_WORKERS ||
        config->consumers == 0 || config->consumers > PC_MAX_WORKERS ||
        config->items_per_producer > PC_MAX_TOTAL_ITEMS ||
        config->producer_delay_ms > PC_MAX_DELAY_MS ||
        config->consumer_delay_ms > PC_MAX_DELAY_MS) {
        return 0;
    }
    size_t capacity = config->capacity, items = config->items_per_producer;
    /* Check P*K before multiplying; then add C STOPs and both operations. */
    if (items != 0 && config->producers > PC_MAX_TOTAL_ITEMS / items) {
        return 0;
    }
    size_t total = config->producers * items;
    size_t entries = total + config->consumers;
    return capacity <= SIZE_MAX / sizeof(struct item) &&
           capacity <= SIZE_MAX / sizeof(uint64_t) &&
           total <= SIZE_MAX / sizeof(unsigned int) &&
           entries <= SIZE_MAX / 2 &&
           2 * entries <= SIZE_MAX / sizeof(struct pc_record) &&
           2 * entries <= UINT64_MAX;
}

static int initialize(struct pc_context *context)
{
    size_t total = context->total_items;
    int error = buffer_init(&context->buffer, context->config.capacity);
    if (error != BUFFER_OK) {
        fprintf(stderr, "buffer_init: %s (%d)\n", buffer_error(error), error);
        return PC_RUNTIME_ERROR;
    }
    context->record_capacity = 2 * (total + context->config.consumers);
    context->result->records = calloc(context->record_capacity, sizeof(struct pc_record));
    context->slot_sequence = calloc(context->config.capacity, sizeof(uint64_t));
    if (total > 0) {
        context->produced = calloc(total, sizeof(unsigned int));
        context->consumed = calloc(total, sizeof(unsigned int));
    }
    if (context->result->records == NULL || context->slot_sequence == NULL ||
        (total > 0 && (context->produced == NULL || context->consumed == NULL))) {
        fprintf(stderr, "pc allocation: insufficient memory\n");
        return PC_RUNTIME_ERROR;
    }
    if (sem_init(&context->empty, 0, (unsigned int)context->config.capacity) == -1) {
        fprintf(stderr, "sem_init empty: %s\n", strerror(errno));
        return PC_RUNTIME_ERROR;
    }
    context->empty_initialized = 1;
    if (sem_init(&context->full, 0, 0) == -1) {
        fprintf(stderr, "sem_init full: %s\n", strerror(errno));
        return PC_RUNTIME_ERROR;
    }
    context->full_initialized = 1;
    if (sem_init(&context->guard, 0, 1) == -1) {
        fprintf(stderr, "sem_init guard: %s\n", strerror(errno));
        return PC_RUNTIME_ERROR;
    }
    context->guard_initialized = 1;
    return PC_OK;
}

static int destroy_semaphore(sem_t *semaphore, int initialized, const char *name)
{
    if (initialized && sem_destroy(semaphore) == -1) {
        fprintf(stderr, "sem_destroy %s: %s\n", name, strerror(errno));
        return PC_RUNTIME_ERROR;
    }
    return PC_OK;
}

static int cleanup(struct pc_context *context)
{
    /* Caller has joined ALL created workers, or has not created any. */
    int status = destroy_semaphore(&context->guard, context->guard_initialized, "guard");
    if (destroy_semaphore(&context->full, context->full_initialized, "full") != PC_OK)
        status = PC_RUNTIME_ERROR;
    if (destroy_semaphore(&context->empty, context->empty_initialized, "empty") != PC_OK)
        status = PC_RUNTIME_ERROR;
    buffer_destroy(&context->buffer);
    free(context->slot_sequence);
    free(context->produced);
    free(context->consumed);
    return status;
}

static void join_checked(pthread_t thread, const char *name)
{
    int error = pthread_join(thread, NULL);
    if (error != 0) {
        fatal(name, error); /* A failed join does NOT prove the worker stopped. */
    }
}

static void verify(struct pc_context *context,
                   const struct worker_args *producers, size_t producers_created,
                   const struct worker_args *consumers, size_t consumers_created)
{
    struct pc_summary *summary = &context->result->summary;
    const struct pc_record *records = context->result->records;
    size_t record_count = context->result->record_count;
    size_t total = context->total_items, items = context->config.items_per_producer;
    /* No workers remain: main may now read the oracle and buffer without guard. */
    size_t produced_total = 0, processed_total = 0;
    for (size_t p = 0; p < producers_created; ++p) {
        summary->producer_items[p] = producers[p].completed;
        produced_total += producers[p].completed;
    }
    for (size_t c = 0; c < consumers_created; ++c) {
        summary->consumer_items[c] = consumers[c].completed;
        processed_total += consumers[c].completed;
    }
    summary->data_processed = processed_total;
    summary->final_count = context->buffer.count;
    /* Compare actual item copies in the two guarded operation streams, too;
     * metadata alone would not detect a swapped/corrupted payload. */
    size_t next_enqueue = 0;
    for (size_t i = 0; i < record_count; ++i) {
        const struct pc_record *out = &records[i];
        if (out->op != PC_DEQUEUE) continue;
        while (next_enqueue < i && records[next_enqueue].op != PC_ENQUEUE)
            ++next_enqueue;
        if (next_enqueue >= i) {
            ++summary->fifo_errors;
            continue;
        }
        const struct pc_record *in = &records[next_enqueue++];
        if (in->item.kind != out->item.kind || in->item.id != out->item.id ||
            in->item.value != out->item.value || in->enqueue_seq != out->enqueue_seq)
            ++summary->fifo_errors;
    }
    /* ID p*K+j must be enqueued by producer p, and each producer's items must
     * be enqueued and dequeued in j order. */
    size_t next_in[PC_MAX_WORKERS] = {0}, next_out[PC_MAX_WORKERS] = {0};
    for (size_t i = 0; i < record_count; ++i) {
        const struct pc_record *r = &records[i];
        if (r->item.kind != ITEM_DATA || r->item.id >= total) continue;
        size_t owner = (size_t)(r->item.id / items), j = (size_t)(r->item.id % items);
        if (r->op == PC_ENQUEUE) {
            if (r->actor != PC_PRODUCER || r->actor_index != owner || j != next_in[owner])
                ++summary->order_errors;
            next_in[owner] = j + 1;
        } else {
            if (r->actor != PC_CONSUMER || r->actor_index >= consumers_created ||
                j != next_out[owner])
                ++summary->order_errors;
            next_out[owner] = j + 1;
        }
    }
    for (size_t id = 0; id < total; ++id) {
        if (context->produced[id] == 0 || context->consumed[id] == 0) ++summary->missing;
        if (context->produced[id] > 1 || context->consumed[id] > 1) ++summary->duplicate;
    }
    int one_stop_each = 1;
    for (size_t c = 0; c < consumers_created; ++c) {
        if (summary->consumer_stops[c] != 1) one_stop_each = 0;
    }
    const struct pc_config *config = &context->config;
    summary->verified = summary->producers_created == config->producers &&
        summary->consumers_created == config->consumers &&
        summary->producers_joined == config->producers &&
        summary->consumers_joined == config->consumers &&
        summary->data_enqueued == total && summary->data_dequeued == total &&
        produced_total == total && processed_total == total &&
        summary->missing == 0 && summary->duplicate == 0 &&
        summary->out_of_range == 0 && summary->value_errors == 0 &&
        summary->fifo_errors == 0 && summary->order_errors == 0 &&
        summary->count_errors == 0 && summary->final_count == 0 &&
        summary->stop_enqueued == config->consumers &&
        summary->stop_dequeued == config->consumers && one_stop_each &&
        record_count == context->record_capacity;
}

int pc_run(const struct pc_config *config, struct pc_result *result)
{
    if (result == NULL || result->records != NULL || !config_valid(config)) {
        fprintf(stderr, "pc configuration: require N=1..65536, P,C=1..64, "
                "K=0..1000000 with P*K<=1000000, delays=0..60000ms, zero result\n");
        return PC_CONFIG_ERROR;
    }
    *result = (struct pc_result){.config = *config};
    struct pc_context context = {
        .config = *config, .result = result,
        .total_items = config->producers * config->items_per_producer
    };
    int status = initialize(&context);
    if (status != PC_OK) {
        cleanup(&context);
        return status;
    }
    /* Handles and per-worker arguments live in this frame until every join. */
    pthread_t consumers[PC_MAX_WORKERS], producers[PC_MAX_WORKERS];
    struct worker_args consumer_args[PC_MAX_WORKERS], producer_args[PC_MAX_WORKERS];
    struct pc_summary *summary = &result->summary;
    size_t consumers_created = 0, producers_created = 0;
    for (; consumers_created < config->consumers; ++consumers_created) {
        size_t c = consumers_created;
        consumer_args[c] = (struct worker_args){&context, c, 0};
        int error = pthread_create(&consumers[c], NULL, consumer, &consumer_args[c]);
        if (error != 0) {
            fprintf(stderr, "pthread_create consumer %zu: %s\n", c, strerror(error));
            status = PC_RUNTIME_ERROR;
            break;
        }
    }
    summary->consumers_created = consumers_created;
    if (consumers_created == 0) {
        cleanup(&context);
        return PC_RUNTIME_ERROR;
    }
    /* A consumer create error means no producer is started at all. */
    for (; status == PC_OK && producers_created < config->producers; ++producers_created) {
        size_t p = producers_created;
        producer_args[p] = (struct worker_args){&context, p, 0};
        int error = pthread_create(&producers[p], NULL, producer, &producer_args[p]);
        if (error != 0) {
            fprintf(stderr, "pthread_create producer %zu: %s\n", p, strerror(error));
            status = PC_RUNTIME_ERROR;
            break;
        }
    }
    summary->producers_created = producers_created;
    for (size_t p = 0; p < producers_created; ++p) {
        join_checked(producers[p], "pthread_join producer");
        ++summary->producers_joined;
    }
    /* No producer remains, so every DATA still queued precedes every STOP.
     * Consumers may still be active: even this read of count needs guard. */
    take_guard(&context, NULL);
    summary->data_pending_after_producer_join = context.buffer.count;
    post_checked(&context.guard, "sem_post guard after producer join snapshot");
    /* One STOP per created consumer; each consumer exits at its first STOP. */
    for (size_t c = 0; c < consumers_created; ++c) {
        enqueue(&context, (struct item){ITEM_STOP, 0, 0}, PC_MAIN, 0);
    }
    for (size_t c = 0; c < consumers_created; ++c) {
        join_checked(consumers[c], "pthread_join consumer");
        ++summary->consumers_joined;
    }
    verify(&context, producer_args, producers_created, consumer_args, consumers_created);
    if (!summary->verified) status = PC_RUNTIME_ERROR;
    if (cleanup(&context) != PC_OK) status = PC_RUNTIME_ERROR;
    return status;
}

void pc_result_destroy(struct pc_result *result)
{
    if (result != NULL) {
        free(result->records);
        *result = (struct pc_result){0};
    }
}
