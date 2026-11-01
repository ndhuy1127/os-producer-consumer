#include "runtime_log.h"

#include <errno.h>
#include <inttypes.h>

static int record(FILE *output, const struct pc_record *r)
{
    if ((r->op != PC_ENQUEUE && r->op != PC_DEQUEUE) ||
        (r->actor != PC_PRODUCER && r->actor != PC_CONSUMER && r->actor != PC_MAIN) ||
        r->actor_index >= PC_MAX_WORKERS ||
        (r->item.kind != ITEM_DATA && r->item.kind != ITEM_STOP) ||
        (r->wait_reason != PC_WAIT_NONE && r->wait_reason != PC_WAIT_SLOT &&
         r->wait_reason != PC_WAIT_ITEM)) {
        errno = EINVAL;
        return PC_RUNTIME_ERROR;
    }
    const char *wait = r->wait_reason == PC_WAIT_NONE ? "NONE" :
                       r->wait_reason == PC_WAIT_SLOT ? "WAIT_SLOT" : "WAIT_ITEM";
    if (fprintf(output, "op_seq=%" PRIu64 " op=%s actor=", r->op_seq,
                r->op == PC_ENQUEUE ? "ENQUEUE" : "DEQUEUE") < 0) return PC_RUNTIME_ERROR;
    if (r->actor == PC_MAIN) {
        if (fputs("MAIN", output) == EOF) return PC_RUNTIME_ERROR;
    } else if (fprintf(output, "%c%zu", r->actor == PC_PRODUCER ? 'P' : 'C',
                       r->actor_index) < 0) return PC_RUNTIME_ERROR;
    if (fprintf(output, " kind=%s data_id=", r->item.kind == ITEM_DATA ? "DATA" : "STOP") < 0)
        return PC_RUNTIME_ERROR;
    if (r->item.kind == ITEM_DATA) {
        if (fprintf(output, "%" PRIu64, r->item.id) < 0) return PC_RUNTIME_ERROR;
    } else if (fputs("-", output) == EOF) return PC_RUNTIME_ERROR;
    if (fprintf(output, " value=%d enqueue_seq=%" PRIu64 " count_after=%zu capacity=%zu wait_reason=%s\n",
                r->item.value, r->enqueue_seq, r->count_after, r->capacity, wait) < 0)
        return PC_RUNTIME_ERROR;
    return PC_OK;
}

/* "label: P0=20 P1=20"; consumers add "/STOP" count: "C0=13/1". */
static int per_worker(FILE *output, const char *label, char prefix, size_t workers,
                      const size_t *items, const size_t *stops)
{
    if (workers > PC_MAX_WORKERS) workers = PC_MAX_WORKERS;
    if (fputs(label, output) == EOF) return PC_RUNTIME_ERROR;
    for (size_t i = 0; i < workers; ++i) {
        if (fprintf(output, " %c%zu=%zu", prefix, i, items[i]) < 0) return PC_RUNTIME_ERROR;
        if (stops != NULL && fprintf(output, "/%zu", stops[i]) < 0) return PC_RUNTIME_ERROR;
    }
    return fputs("\n", output) == EOF ? PC_RUNTIME_ERROR : PC_OK;
}

int runtime_log_result(FILE *output, const struct pc_result *result, int run_status, int quiet)
{
    if (output == NULL || result == NULL || (result->record_count != 0 && result->records == NULL)) {
        errno = EINVAL;
        return PC_RUNTIME_ERROR;
    }
    const struct pc_config *c = &result->config;
    const struct pc_summary *s = &result->summary;
    if (fprintf(output, "config: N=%zu P=%zu C=%zu K=%zu Dp_ms=%u Dc_ms=%u\n",
                c->capacity, c->producers, c->consumers, c->items_per_producer,
                c->producer_delay_ms, c->consumer_delay_ms) < 0) return PC_RUNTIME_ERROR;
    if (!quiet) {
        for (size_t i = 0; i < result->record_count; ++i)
            if (record(output, &result->records[i]) != PC_OK) return PC_RUNTIME_ERROR;
    }
    if (fprintf(output, "DATA: enqueued=%zu dequeued=%zu processed=%zu missing=%zu duplicate=%zu out_of_range=%zu value_errors=%zu FIFO_errors=%zu order_errors=%zu\n",
                s->data_enqueued, s->data_dequeued, s->data_processed, s->missing,
                s->duplicate, s->out_of_range, s->value_errors, s->fifo_errors,
                s->order_errors) < 0)
        return PC_RUNTIME_ERROR;
    if (fprintf(output, "buffer: min=%zu max=%zu final=%zu count_errors=%zu; STOP: enqueued=%zu dequeued=%zu\n",
                s->min_count, s->max_count, s->final_count, s->count_errors,
                s->stop_enqueued, s->stop_dequeued) < 0) return PC_RUNTIME_ERROR;
    if (fprintf(output, "waits: WAIT_SLOT=%zu WAIT_ITEM=%zu records=%zu data_pending_after_producer_join=%zu\n",
                s->wait_slot, s->wait_item, result->record_count,
                s->data_pending_after_producer_join) < 0) return PC_RUNTIME_ERROR;
    if (per_worker(output, "producers(DATA):", 'P', s->producers_created,
                   s->producer_items, NULL) != PC_OK ||
        per_worker(output, "consumers(DATA/STOP):", 'C', s->consumers_created,
                   s->consumer_items, s->consumer_stops) != PC_OK) return PC_RUNTIME_ERROR;
    if (fprintf(output, "workers: created=%zuP/%zuC joined=%zuP/%zuC; verification=%s run_status=%d\n",
                s->producers_created, s->consumers_created, s->producers_joined,
                s->consumers_joined, run_status == PC_OK && s->verified ? "PASS" : "FAIL",
                run_status) < 0 || fflush(output) == EOF) return PC_RUNTIME_ERROR;
    return PC_OK;
}
