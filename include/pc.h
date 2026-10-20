#ifndef OS_PC_PC_H
#define OS_PC_PC_H

#include <stddef.h>
#include <stdint.h>
#include "item.h"

/* Limits shared by the core and the CLI parser. */
#define PC_MAX_CAPACITY 65536
#define PC_MAX_WORKERS 64          /* Per kind: producers and consumers. */
#define PC_MAX_TOTAL_ITEMS 1000000 /* P*K DATA items in one run. */
#define PC_MAX_DELAY_MS 60000

enum pc_status { PC_OK = 0, PC_RUNTIME_ERROR = 1, PC_CONFIG_ERROR = 2 };
enum pc_operation { PC_ENQUEUE, PC_DEQUEUE };
enum pc_actor { PC_PRODUCER, PC_CONSUMER, PC_MAIN };
enum pc_wait_reason { PC_WAIT_NONE, PC_WAIT_SLOT, PC_WAIT_ITEM };

struct pc_config {
    size_t capacity;
    size_t producers;
    size_t consumers;
    size_t items_per_producer;
    unsigned int producer_delay_ms;
    unsigned int consumer_delay_ms;
};

struct pc_record {
    uint64_t op_seq;
    enum pc_operation op;
    enum pc_actor actor;
    struct item item;
    uint64_t enqueue_seq;
    size_t count_after;
    size_t capacity;
    enum pc_wait_reason wait_reason;
    size_t actor_index; /* Producer p or consumer c; 0 for MAIN. */
};

struct pc_summary {
    size_t data_enqueued, data_dequeued, data_processed;
    size_t stop_enqueued, stop_dequeued;
    size_t missing, duplicate, out_of_range, value_errors, fifo_errors;
    size_t count_errors, min_count, max_count, final_count;
    size_t wait_slot, wait_item;
    size_t producers_created, consumers_created;
    size_t producers_joined, consumers_joined;
    size_t data_pending_after_producer_join;
    int verified;
    /* DATA not owned by the enqueuing producer, or out of j order per producer. */
    size_t order_errors;
    size_t producer_items[PC_MAX_WORKERS]; /* DATA enqueued by producer p. */
    size_t consumer_items[PC_MAX_WORKERS]; /* DATA processed by consumer c. */
    size_t consumer_stops[PC_MAX_WORKERS]; /* STOP dequeued by consumer c. */
};

struct pc_result {
    struct pc_config config;
    struct pc_summary summary;
    struct pc_record *records;
    size_t record_count;
};

struct pc_config pc_default_config(void);
/* Synchronous, finite run with P producers and C consumers (1..64 each).
 * Producer p enqueues IDs p*K..p*K+K-1; main joins all producers, then
 * enqueues one STOP per consumer and joins all consumers.
 * result must be zero-initialized and destroyed before reuse; never copy an
 * owning result. Records (including STOP) remain available after all joins.
 * Recoverable failures return 1; invalid config returns 2 before workers.
 * Irrecoverable synchronization/join failure terminates the process with 1
 * rather than leave another worker waiting or destroy possibly live storage.
 */
int pc_run(const struct pc_config *config, struct pc_result *result);
void pc_result_destroy(struct pc_result *result);

#endif
