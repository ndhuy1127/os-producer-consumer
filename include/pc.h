#ifndef OS_PC_PC_H
#define OS_PC_PC_H

#include <stddef.h>
#include <stdint.h>
#include "item.h"

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
    enum pc_actor actor; /* P0, C0, or MAIN in the Week 2 implementation. */
    struct item item;
    uint64_t enqueue_seq;
    size_t count_after;
    size_t capacity;
    enum pc_wait_reason wait_reason;
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
};

struct pc_result {
    struct pc_config config;
    struct pc_summary summary;
    struct pc_record *records;
    size_t record_count;
};

struct pc_config pc_default_config(void);
/* Synchronous, finite run. Week 2 accepts only producers=consumers=1.
 * result must be zero-initialized and destroyed before reuse; never copy an
 * owning result. Records (including STOP) remain available after all joins.
 * Recoverable failures return 1; invalid config returns 2 before workers.
 * Irrecoverable synchronization/join failure terminates the process with 1
 * rather than leave another worker waiting or destroy possibly live storage.
 */
int pc_run(const struct pc_config *config, struct pc_result *result);
void pc_result_destroy(struct pc_result *result);

#endif
