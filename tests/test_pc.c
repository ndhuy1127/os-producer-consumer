#include "pc.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __func__, __LINE__, #expression); \
        ok = 0; goto done; \
    } \
} while (0)

struct test_case {
    const char *name;
    struct pc_config config;
    int need_full, need_empty, need_pending;
};

static const struct test_case cases[] = {
    {"default", {8, 1, 1, 20, 0, 0}, 0, 0, 0},
    {"source-N5", {5, 1, 1, 20, 0, 0}, 0, 0, 0},
    {"T02", {8, 1, 1, 100, 0, 0}, 0, 0, 0},
    {"T03", {1, 1, 1, 100, 0, 0}, 0, 0, 0},
    {"T04", {2, 1, 1, 40, 0, 20}, 1, 0, 0},
    {"T05", {2, 1, 1, 40, 20, 0}, 0, 1, 0},
    {"K0", {1, 1, 1, 0, 0, 0}, 0, 0, 0},
    {"queued-stop", {8, 1, 1, 20, 0, 20}, 1, 0, 1},
    /* Week 3: several producers/consumers. */
    {"T06a", {3, 3, 2, 100, 0, 0}, 0, 0, 0},
    {"T06b", {2, 2, 5, 100, 0, 0}, 0, 0, 0},
    {"T07", {16, 4, 3, 10000, 0, 0}, 0, 0, 0},
    {"T09a", {1, 1, 4, 0, 0, 0}, 0, 0, 0},
    {"T09b", {2, 2, 3, 30, 0, 10}, 0, 0, 0},
    {"T04-multi", {2, 3, 2, 40, 0, 20}, 1, 0, 0},
    {"T05-multi", {2, 2, 3, 40, 20, 0}, 0, 1, 0},
    {"queued-stop-multi", {8, 2, 3, 40, 0, 20}, 1, 0, 1}
};

static void print_record(const struct pc_record *r)
{
    const char *wait = r->wait_reason == PC_WAIT_NONE ? "NONE" :
                       r->wait_reason == PC_WAIT_SLOT ? "WAIT_SLOT" : "WAIT_ITEM";
    printf("op_seq=%" PRIu64 " op=%s actor=",
           r->op_seq, r->op == PC_ENQUEUE ? "ENQUEUE" : "DEQUEUE");
    if (r->actor == PC_MAIN) printf("MAIN");
    else printf("%c%zu", r->actor == PC_PRODUCER ? 'P' : 'C', r->actor_index);
    printf(" kind=%s data_id=", r->item.kind == ITEM_DATA ? "DATA" : "STOP");
    if (r->item.kind == ITEM_DATA) printf("%" PRIu64, r->item.id);
    else printf("-");
    printf(" value=%d enqueue_seq=%" PRIu64 " count_after=%zu capacity=%zu wait_reason=%s\n",
           r->item.value, r->enqueue_seq, r->count_after, r->capacity, wait);
}

static int run_case(const struct test_case *test, int records)
{
    const struct pc_config *c = &test->config;
    size_t k = c->items_per_producer, total = c->producers * k;
    struct pc_result result = {0};
    /* Linear append-only oracle, independent of the circular buffer/metadata. */
    struct item *queue = calloc(total + c->consumers, sizeof(*queue));
    unsigned int *pseen = calloc(total + 1, sizeof(*pseen));
    unsigned int *cseen = calloc(total + 1, sizeof(*cseen));
    size_t next_in[PC_MAX_WORKERS] = {0}, next_out[PC_MAX_WORKERS] = {0};
    size_t taken_by[PC_MAX_WORKERS] = {0}, stops_by[PC_MAX_WORKERS] = {0};
    size_t added = 0, taken = 0, data_in = 0, data_out = 0;
    size_t stop_in = 0, stop_out = 0, full = 0, empty = 0, slots = 0, items = 0;
    size_t max_count = 0;
    int ok = 1;
    printf("case=%s N=%zu P=%zu C=%zu K=%zu Dp_ms=%u Dc_ms=%u\n",
           test->name, c->capacity, c->producers, c->consumers,
           k, c->producer_delay_ms, c->consumer_delay_ms);
    REQUIRE(queue != NULL && pseen != NULL && cseen != NULL);
    int status = pc_run(c, &result);
    printf("pc_run_status=%d records=%zu\n", status, result.record_count);
    REQUIRE(status == PC_OK);
    REQUIRE(result.record_count == 2 * (total + c->consumers));
    for (size_t i = 0; i < result.record_count; ++i) {
        const struct pc_record *r = &result.records[i];
        if (records) print_record(r);
        REQUIRE(r->op_seq == i + 1 && r->capacity == c->capacity);
        REQUIRE(r->item.kind == ITEM_DATA || r->item.kind == ITEM_STOP);
        if (r->item.kind == ITEM_DATA) {
            REQUIRE(r->item.id < total && r->item.value == (int)(r->item.id % 1000U) + 1);
        }
        size_t owner = k == 0 ? 0 : (size_t)(r->item.id / k);
        size_t j = k == 0 ? 0 : (size_t)(r->item.id % k);
        if (r->op == PC_ENQUEUE) {
            REQUIRE(added < total + c->consumers);
            REQUIRE(r->enqueue_seq == added + 1);
            REQUIRE(r->wait_reason == PC_WAIT_NONE || r->wait_reason == PC_WAIT_SLOT);
            if (r->wait_reason == PC_WAIT_SLOT) ++slots;
            queue[added++] = r->item;
            if (r->item.kind == ITEM_DATA) {
                /* ID p*K+j belongs to producer p, enqueued in j order. */
                REQUIRE(stop_in == 0 && r->actor == PC_PRODUCER);
                REQUIRE(r->actor_index == owner && j == next_in[owner]);
                ++next_in[owner];
                ++pseen[r->item.id]; ++data_in;
            } else {
                REQUIRE(r->actor == PC_MAIN && r->actor_index == 0 && data_in == total);
                ++stop_in;
            }
        } else {
            REQUIRE(r->op == PC_DEQUEUE && r->actor == PC_CONSUMER);
            REQUIRE(r->actor_index < c->consumers && taken < added);
            REQUIRE(r->enqueue_seq == taken + 1);
            REQUIRE(r->wait_reason == PC_WAIT_NONE || r->wait_reason == PC_WAIT_ITEM);
            if (r->wait_reason == PC_WAIT_ITEM) ++items;
            struct item expected = queue[taken++];
            REQUIRE(r->item.kind == expected.kind && r->item.id == expected.id &&
                    r->item.value == expected.value);
            if (r->item.kind == ITEM_DATA) {
                REQUIRE(stop_out == 0 && j == next_out[owner]);
                ++next_out[owner];
                ++cseen[r->item.id]; ++data_out;
                ++taken_by[r->actor_index];
            } else {
                REQUIRE(data_out == total);
                ++stop_out;
                ++stops_by[r->actor_index];
            }
        }
        REQUIRE(added >= taken && added - taken <= c->capacity);
        REQUIRE(r->count_after == added - taken);
        if (r->count_after == c->capacity) ++full;
        if (r->count_after == 0) ++empty;
        if (r->count_after > max_count) max_count = r->count_after;
    }
    for (size_t id = 0; id < total; ++id)
        REQUIRE(pseen[id] == 1 && cseen[id] == 1);
    REQUIRE(added == taken && data_in == total && data_out == data_in);
    REQUIRE(stop_in == c->consumers && stop_out == c->consumers);
    const struct pc_summary *s = &result.summary;
    REQUIRE(s->verified && s->missing == 0 && s->duplicate == 0 &&
            s->out_of_range == 0 && s->value_errors == 0 && s->fifo_errors == 0 &&
            s->order_errors == 0 && s->count_errors == 0 && s->final_count == 0);
    REQUIRE(s->data_enqueued == data_in && s->data_dequeued == data_out &&
            s->data_processed == data_out && s->stop_enqueued == stop_in &&
            s->stop_dequeued == stop_out && s->wait_slot == slots && s->wait_item == items &&
            s->min_count == 0 && s->max_count == max_count);
    REQUIRE(s->producers_created == c->producers && s->consumers_created == c->consumers &&
            s->producers_joined == c->producers && s->consumers_joined == c->consumers);
    for (size_t p = 0; p < c->producers; ++p)
        REQUIRE(next_in[p] == k && next_out[p] == k && s->producer_items[p] == k);
    for (size_t q = 0; q < c->consumers; ++q)
        REQUIRE(stops_by[q] == 1 && s->consumer_stops[q] == 1 &&
                s->consumer_items[q] == taken_by[q]);
    if (test->need_full) REQUIRE(full > 0 && slots > 0);
    if (test->need_empty) REQUIRE(empty > 0 && items > 0);
    if (test->need_pending) REQUIRE(s->data_pending_after_producer_join > 0);
    printf("oracle: DATA=%zu/%zu STOP=%zu/%zu IDs_once=PASS FIFO=PASS per_producer_order=PASS "
           "count=[0,%zu] final=0 workers=create%zu/join%zu\n",
           data_in, data_out, stop_in, stop_out, max_count,
           c->producers + c->consumers, c->producers + c->consumers);
    printf("observed: full_snapshots=%zu empty_snapshots=%zu WAIT_SLOT=%zu WAIT_ITEM=%zu "
           "data_pending_after_producer_join=%zu consumer_DATA=",
           full, empty, slots, items, s->data_pending_after_producer_join);
    for (size_t q = 0; q < c->consumers; ++q)
        printf("%s%zu", q == 0 ? "" : ",", taken_by[q]);
    printf("\n");
done:
    free(queue); free(pseen); free(cseen);
    pc_result_destroy(&result);
    printf("%s case=%s\n", ok ? "PASS" : "FAIL", test->name);
    return ok;
}

int main(int argc, char **argv)
{
    const char *name = argc > 1 ? argv[1] : "all";
    int records = argc == 3 && strcmp(argv[2], "--records") == 0;
    if (argc > 3 || (argc == 3 && !records)) return PC_CONFIG_ERROR;
    size_t ran = 0, passed = 0;
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        if (strcmp(name, "all") == 0 || strcmp(name, cases[i].name) == 0) {
            ++ran;
            passed += (size_t)run_case(&cases[i], records);
        }
    }
    if (ran == 0) {
        fprintf(stderr, "Unknown test preset: %s\n", name);
        return PC_CONFIG_ERROR;
    }
    printf("Core preset tests: %zu/%zu %s\n", passed, ran, passed == ran ? "PASS" : "FAIL");
    return passed == ran ? PC_OK : PC_RUNTIME_ERROR;
}
