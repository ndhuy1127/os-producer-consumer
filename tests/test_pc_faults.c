#define _POSIX_C_SOURCE 200809L
#include "pc.h"
#include "buffer.h"

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* GNU ld wrappers are linked only into pc-error-test. No production hooks. */
int __real_sem_init(sem_t *, int, unsigned int);
int __real_sem_wait(sem_t *);
int __real_sem_trywait(sem_t *);
int __real_sem_post(sem_t *);
int __real_sem_destroy(sem_t *);
int __real_pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
int __real_pthread_join(pthread_t, void **);
void *__real_calloc(size_t, size_t);
void *__real_malloc(size_t);
void __real_free(void *);
int __real_buffer_push(struct buffer *, struct item);
int __real_buffer_pop(struct buffer *, struct item *);

static const char *fault;
static size_t init_calls, init_successes, destroy_calls, create_calls, create_successes, join_calls;
static size_t calloc_calls;
static atomic_size_t live_allocations;
static atomic_int injected, permission_eintr, guard_eintr, consumer_wait_seen;
static sem_t *full_sem, *guard_sem;
static sem_t consumer_ready;

static int is_fault(const char *name) { return strcmp(fault, name) == 0; }

int __wrap_sem_init(sem_t *s, int shared, unsigned int value)
{
    ++init_calls;
    if (init_calls == 2) full_sem = s;
    if (init_calls == 3) guard_sem = s;
    if ((is_fault("init-empty") && init_calls == 1) ||
        (is_fault("init-full") && init_calls == 2) ||
        (is_fault("init-guard") && init_calls == 3)) {
        errno = ENOSPC; return -1;
    }
    if (shared != 0 || (init_calls == 2 && value != 0) ||
        (init_calls == 3 && value != 1)) abort();
    int status = __real_sem_init(s, shared, value);
    if (status == 0) ++init_successes;
    return status;
}

int __wrap_sem_wait(sem_t *s)
{
    if (is_fault("wait-eintr")) {
        atomic_int *flag = s == guard_sem ? &guard_eintr : &permission_eintr;
        if (atomic_exchange(flag, 1) == 0) {
            errno = EINTR; return -1;
        }
    }
    if (s == full_sem && (is_fault("producer-create") || is_fault("wait-eintr")) &&
        atomic_exchange(&consumer_wait_seen, 1) == 0) {
        if (__real_sem_post(&consumer_ready) == -1) abort();
    }
    if (is_fault("fatal-guard") && s == guard_sem) { errno = EINVAL; return -1; }
    if (is_fault("fatal-wait") && s == full_sem) { errno = EINVAL; return -1; }
    return __real_sem_wait(s);
}

int __wrap_sem_trywait(sem_t *s)
{
    if (is_fault("trywait-eintr") && atomic_exchange(&injected, 1) == 0) {
        errno = EINTR; return -1;
    }
    if (is_fault("fatal-trywait")) { errno = EINVAL; return -1; }
    return __real_sem_trywait(s);
}

int __wrap_sem_post(sem_t *s)
{
    if (is_fault("fatal-post") && s == full_sem) { errno = EINVAL; return -1; }
    return __real_sem_post(s);
}

int __wrap_sem_destroy(sem_t *s)
{
    if (join_calls != create_successes) abort(); /* Never destroy before joins. */
    ++destroy_calls;
    int status = __real_sem_destroy(s);
    if (status == 0 && is_fault("destroy")) { errno = EINVAL; return -1; }
    return status;
}

int __wrap_pthread_create(pthread_t *t, const pthread_attr_t *attr,
                          void *(*start)(void *), void *arg)
{
    ++create_calls;
    if (create_calls == 2 && (is_fault("producer-create") || is_fault("wait-eintr") ||
                              is_fault("fatal-wait"))) {
        /* Consumer has observed EAGAIN and entered the sem_wait(full) path;
         * no DATA/STOP has been posted. No sleep/polling to guess readiness. */
        while (__real_sem_wait(&consumer_ready) == -1) {
            if (errno != EINTR) abort();
        }
        if (is_fault("producer-create")) return EAGAIN;
    }
    if (is_fault("consumer-create") && create_calls == 1) return EAGAIN;
    int error = __real_pthread_create(t, attr, start, arg);
    if (error == 0) ++create_successes;
    return error;
}

int __wrap_pthread_join(pthread_t t, void **value)
{
    if (is_fault("fatal-join")) return EINVAL;
    int error = __real_pthread_join(t, value);
    if (error == 0) ++join_calls;
    return error;
}

void *__wrap_malloc(size_t size)
{
    if (is_fault("malloc")) return NULL;
    void *p = __real_malloc(size);
    if (p != NULL) atomic_fetch_add(&live_allocations, 1);
    return p;
}

void *__wrap_calloc(size_t n, size_t size)
{
    ++calloc_calls;
    size_t fail_at = is_fault("calloc-record") ? 1 : is_fault("calloc-slot") ? 2 :
                     is_fault("calloc-produced") ? 3 : is_fault("calloc-consumed") ? 4 : 0;
    if (calloc_calls == fail_at) return NULL;
    void *p = __real_calloc(n, size);
    if (p != NULL) atomic_fetch_add(&live_allocations, 1);
    return p;
}

void __wrap_free(void *p)
{
    if (p != NULL) atomic_fetch_sub(&live_allocations, 1);
    __real_free(p);
}

int __wrap_buffer_push(struct buffer *b, struct item item)
{
    if (is_fault("fatal-push")) return BUFFER_INVALID_ARGUMENT;
    return __real_buffer_push(b, item);
}

int __wrap_buffer_pop(struct buffer *b, struct item *item)
{
    if (is_fault("fatal-pop")) return BUFFER_INVALID_ARGUMENT;
    return __real_buffer_pop(b, item);
}

int main(int argc, char **argv)
{
    if (argc != 2) return PC_CONFIG_ERROR;
    fault = argv[1];
    if (__real_sem_init(&consumer_ready, 0, 0) == -1) return PC_RUNTIME_ERROR;
    struct pc_config config = pc_default_config();
    struct pc_result result = {0};
    int expected = PC_RUNTIME_ERROR;
    if (is_fault("wait-eintr") || is_fault("trywait-eintr")) expected = PC_OK;
    if (is_fault("config-N0")) { config.capacity = 0; expected = PC_CONFIG_ERROR; }
    if (is_fault("config-Nbig")) { config.capacity = 65537; expected = PC_CONFIG_ERROR; }
    if (is_fault("config-P2")) { config.producers = 2; expected = PC_CONFIG_ERROR; }
    if (is_fault("config-C2")) { config.consumers = 2; expected = PC_CONFIG_ERROR; }
    if (is_fault("config-Kbig")) { config.items_per_producer = SIZE_MAX; expected = PC_CONFIG_ERROR; }
    if (is_fault("config-delay")) { config.consumer_delay_ms = 60001; expected = PC_CONFIG_ERROR; }
    int status = pc_run(&config, &result);
    int ok = status == expected && create_successes == join_calls &&
             init_successes == destroy_calls;
    if (expected == PC_CONFIG_ERROR) ok = ok && create_calls == 0 && init_calls == 0;
    if (expected == PC_OK) ok = ok && result.summary.verified;
    if (is_fault("wait-eintr"))
        ok = ok && atomic_load(&guard_eintr) && atomic_load(&permission_eintr);
    if (is_fault("trywait-eintr")) ok = ok && atomic_load(&injected);
    if (is_fault("producer-create"))
        ok = ok && atomic_load(&consumer_wait_seen) && create_successes == 1 &&
             result.summary.producers_created == 0 && result.summary.consumers_joined == 1 &&
             result.summary.stop_enqueued == 1 && result.summary.stop_dequeued == 1 &&
             result.summary.final_count == 0 && result.summary.data_enqueued == 0;
    pc_result_destroy(&result);
    ok = ok && atomic_load(&live_allocations) == 0;
    if (__real_sem_destroy(&consumer_ready) == -1) ok = 0;
    printf("%s fault=%s pc_run=%d expected=%d creates=%zu joins=%zu sem_init=%zu sem_destroy=%zu live_allocations=%zu consumer_wait_path=%d guard_EINTR=%d permission_EINTR=%d\n",
           ok ? "PASS" : "FAIL", fault, status, expected, create_successes,
           join_calls, init_successes, destroy_calls, atomic_load(&live_allocations),
           atomic_load(&consumer_wait_seen), atomic_load(&guard_eintr),
           atomic_load(&permission_eintr));
    return ok ? PC_OK : PC_RUNTIME_ERROR;
}
