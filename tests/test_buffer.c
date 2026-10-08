#include "buffer.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef BUFFER_TEST_WRAP_ALLOC
/* GNU ld wrappers affect only direct calls from the linked objects.
 * They let us test malloc failure and the array's lifetime deterministically.
 */
void *__real_malloc(size_t size);
void __real_free(void *pointer);
static int fail_next_allocation;
static size_t live_allocations;

void *__wrap_malloc(size_t size)
{
    if (fail_next_allocation) {
        fail_next_allocation = 0;
        return NULL;
    }
    void *pointer = __real_malloc(size);
    if (pointer != NULL) {
        ++live_allocations;
    }
    return pointer;
}

void __wrap_free(void *pointer)
{
    if (pointer != NULL) {
        --live_allocations;
    }
    __real_free(pointer);
}
#endif

#define CHECK(expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __func__, __LINE__, #expression); \
        return 0; \
    } \
} while (0)

static struct item data(uint64_t id, int value)
{
    return (struct item){ITEM_DATA, id, value};
}

static int same_item(struct item a, struct item b)
{
    return a.kind == b.kind && a.id == b.id && a.value == b.value;
}

static int same_state(struct buffer a, struct buffer b)
{
    return a.items == b.items && a.head == b.head && a.tail == b.tail &&
           a.count == b.count && a.capacity == b.capacity;
}

static int invariant(const struct buffer *b)
{
    return b->items != NULL && b->capacity > 0 && b->count <= b->capacity &&
           b->head < b->capacity && b->tail < b->capacity;
}

static int test_init(void)
{
    struct buffer b = {0};
    CHECK(buffer_init(&b, 3) == BUFFER_OK);
    CHECK(b.head == 0 && b.tail == 0 && b.count == 0 && b.capacity == 3);
    CHECK(invariant(&b));
    buffer_destroy(&b);
    return 1;
}

static int test_copy(void)
{
    struct buffer b = {0};
    struct item input = data(42, -17), output;
    const struct item expected = input;
    CHECK(buffer_init(&b, 2) == BUFFER_OK);
    CHECK(buffer_push(&b, input) == BUFFER_OK);
    input = data(99, 100); /* Mutating the caller's value cannot change the queue. */
    CHECK(buffer_pop(&b, &output) == BUFFER_OK);
    CHECK(same_item(output, expected) && b.count == 0 && invariant(&b));
    buffer_destroy(&b);
    return 1;
}

static int test_fifo_full(void)
{
    const struct item expected[] = {
        {ITEM_DATA, 90, 8}, {ITEM_DATA, 2, -4}, {ITEM_DATA, 77, 31}
    }; /* Deliberately non-monotonic IDs: FIFO is insertion order. */
    struct buffer b = {0};
    CHECK(buffer_init(&b, 3) == BUFFER_OK);
    for (size_t i = 0; i < 3; ++i) {
        CHECK(buffer_push(&b, expected[i]) == BUFFER_OK && invariant(&b));
    }
    struct buffer before = b;
    CHECK(buffer_push(&b, data(999, 0)) == BUFFER_FULL);
    CHECK(same_state(b, before));
    for (size_t i = 0; i < 3; ++i) {
        struct item output;
        CHECK(same_item(b.items[i], expected[i]));
        CHECK(buffer_pop(&b, &output) == BUFFER_OK);
        CHECK(same_item(output, expected[i]) && invariant(&b));
    }
    buffer_destroy(&b);
    return 1;
}

static int test_empty(void)
{
    struct buffer b = {0};
    struct item output = data(876, -99), sentinel = output;
    CHECK(buffer_init(&b, 2) == BUFFER_OK);
    struct buffer before = b;
    CHECK(buffer_pop(&b, &output) == BUFFER_EMPTY);
    CHECK(same_state(b, before) && same_item(output, sentinel));
    CHECK(buffer_push(&b, data(11, 12)) == BUFFER_OK);
    CHECK(buffer_pop(&b, &output) == BUFFER_OK);
    before = b;
    output = sentinel;
    CHECK(buffer_pop(&b, &output) == BUFFER_EMPTY);
    CHECK(same_state(b, before) && same_item(output, sentinel));
    buffer_destroy(&b);
    return 1;
}

static int test_wrap(void)
{
    /* Independent linear FIFO oracle: no modulo/head/tail from the code under test. */
    struct item expected[3];
    size_t length = 0;
    uint64_t next_id = 500;
    struct buffer b = {0};
    CHECK(buffer_init(&b, 3) == BUFFER_OK);
    for (size_t round = 0; round < 100; ++round) {
        while (length < 3) {
            struct item input = data(next_id, -(int)next_id);
            ++next_id;
            expected[length++] = input;
            CHECK(buffer_push(&b, input) == BUFFER_OK && invariant(&b));
        }
        /* Leave one queued item so wrap-around also happens while nonempty. */
        for (size_t i = 0; i < 2; ++i) {
            struct item output;
            CHECK(buffer_pop(&b, &output) == BUFFER_OK);
            CHECK(same_item(output, expected[0]) && invariant(&b));
            for (size_t j = 1; j < length; ++j) {
                expected[j - 1] = expected[j];
            }
            --length;
            CHECK(b.count == length);
        }
    }
    struct item output;
    CHECK(buffer_pop(&b, &output) == BUFFER_OK && same_item(output, expected[0]));
    CHECK(b.count == 0 && invariant(&b));
    buffer_destroy(&b);
    return 1;
}

static int test_capacity_one(void)
{
    struct buffer b = {0};
    CHECK(buffer_init(&b, 1) == BUFFER_OK);
    for (uint64_t i = 0; i < 50; ++i) {
        struct item input = data(i, (int)i - 25), output;
        CHECK(buffer_push(&b, input) == BUFFER_OK);
        CHECK(b.head == 0 && b.tail == 0 && b.count == 1);
        CHECK(buffer_push(&b, data(999, 0)) == BUFFER_FULL);
        CHECK(buffer_pop(&b, &output) == BUFFER_OK && same_item(output, input));
        CHECK(b.head == 0 && b.tail == 0 && b.count == 0);
        CHECK(buffer_pop(&b, &output) == BUFFER_EMPTY && same_item(output, input));
    }
    buffer_destroy(&b);
    return 1;
}

static int test_stop(void)
{
    const struct item expected[] = {
        {ITEM_DATA, 0, 0}, {ITEM_STOP, UINT64_MAX, -9}, {ITEM_DATA, 1, 5}
    };
    struct buffer b = {0};
    CHECK(buffer_init(&b, 3) == BUFFER_OK);
    for (size_t i = 0; i < 3; ++i) {
        CHECK(buffer_push(&b, expected[i]) == BUFFER_OK);
    }
    CHECK(b.count == 3);
    for (size_t i = 0; i < 3; ++i) {
        struct item output;
        CHECK(buffer_pop(&b, &output) == BUFFER_OK && same_item(output, expected[i]));
    }
    CHECK(b.count == 0);
    buffer_destroy(&b);
    return 1;
}

static int test_invalid(void)
{
    struct buffer b = {0}, zero = {0};
    struct item input = data(1, 2), output = data(9, 8), sentinel = output;
    CHECK(buffer_init(NULL, 3) == BUFFER_INVALID_ARGUMENT);
    CHECK(buffer_init(&b, 0) == BUFFER_INVALID_ARGUMENT && same_state(b, zero));
    CHECK(buffer_push(NULL, input) == BUFFER_INVALID_ARGUMENT);
    CHECK(buffer_pop(NULL, &output) == BUFFER_INVALID_ARGUMENT);
    CHECK(buffer_push(&b, input) == BUFFER_INVALID_ARGUMENT);
    CHECK(buffer_pop(&b, &output) == BUFFER_INVALID_ARGUMENT);
    CHECK(same_item(output, sentinel) && same_state(b, zero));
    CHECK(buffer_init(&b, 2) == BUFFER_OK);
    CHECK(buffer_push(&b, input) == BUFFER_OK);
    struct buffer before = b;
    CHECK(buffer_pop(&b, NULL) == BUFFER_INVALID_ARGUMENT && same_state(b, before));
    struct item invalid = {(enum item_kind)99, 3, 4};
    CHECK(buffer_push(&b, invalid) == BUFFER_INVALID_ARGUMENT && same_state(b, before));
    CHECK(buffer_pop(&b, &output) == BUFFER_OK && same_item(output, input));
    buffer_destroy(&b);
    return 1;
}

static int test_overflow(void)
{
    struct buffer b = {0}, zero = {0};
    CHECK(buffer_init(&b, SIZE_MAX / sizeof(struct item) + 1) == BUFFER_SIZE_OVERFLOW);
    CHECK(same_state(b, zero));
    buffer_destroy(&b);
    return 1;
}

static int test_reinit_live(void)
{
    struct buffer b = {0};
    struct item output, input = data(12, -12);
    CHECK(buffer_init(&b, 2) == BUFFER_OK);
    CHECK(buffer_push(&b, input) == BUFFER_OK);
    struct buffer before = b;
    CHECK(buffer_init(&b, 4) == BUFFER_ALREADY_INITIALIZED);
    CHECK(same_state(b, before));
    CHECK(buffer_pop(&b, &output) == BUFFER_OK && same_item(output, input));
    buffer_destroy(&b);
    return 1;
}

static int test_destroy(void)
{
    struct buffer b = {0}, zero = {0};
    buffer_destroy(NULL);
    buffer_destroy(&b);
    CHECK(same_state(b, zero));
    CHECK(buffer_init(&b, 4) == BUFFER_OK);
    CHECK(buffer_push(&b, data(4, 7)) == BUFFER_OK);
    buffer_destroy(&b); /* Also releases a nonempty queue. */
    CHECK(same_state(b, zero));
    buffer_destroy(&b);
    struct item output = data(88, 99), sentinel = output;
    CHECK(buffer_pop(&b, &output) == BUFFER_INVALID_ARGUMENT);
    CHECK(same_item(output, sentinel));
    CHECK(buffer_push(&b, sentinel) == BUFFER_INVALID_ARGUMENT);
    CHECK(buffer_init(&b, 1) == BUFFER_OK);
    CHECK(buffer_push(&b, sentinel) == BUFFER_OK);
    CHECK(buffer_pop(&b, &output) == BUFFER_OK && same_item(output, sentinel));
    buffer_destroy(&b);
    return 1;
}

#ifdef BUFFER_TEST_WRAP_ALLOC
static int test_allocation_failure(void)
{
    struct buffer b = {0}, zero = {0};
    fail_next_allocation = 1;
    CHECK(buffer_init(&b, 3) == BUFFER_NO_MEMORY);
    CHECK(same_state(b, zero) && live_allocations == 0);
    buffer_destroy(&b);
    CHECK(buffer_init(&b, 3) == BUFFER_OK && live_allocations == 1);
    buffer_destroy(&b);
    CHECK(live_allocations == 0);
    return 1;
}
#endif

int main(void)
{
    const struct { const char *name; int (*run)(void); } tests[] = {
        {"empty initialization", test_init},
        {"one item and value copy", test_copy},
        {"FIFO and full rejection preserves data", test_fifo_full},
        {"empty rejection preserves state/output", test_empty},
        {"100 wrap-around rounds with independent FIFO oracle", test_wrap},
        {"capacity 1, 50 cycles", test_capacity_one},
        {"STOP is an ordinary FIFO item", test_stop},
        {"invalid arguments and lifecycle", test_invalid},
        {"allocation size overflow", test_overflow},
        {"live reinitialization preserves queue", test_reinit_live},
        {"destroy/repeat/reuse", test_destroy},
#ifdef BUFFER_TEST_WRAP_ALLOC
        {"injected malloc failure and recovery", test_allocation_failure},
#endif
    };
    size_t passed = 0;
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        int ok = tests[i].run();
#ifdef BUFFER_TEST_WRAP_ALLOC
        if (live_allocations != 0) {
            fprintf(stderr, "FAIL %s: %zu unreleased array allocations\n",
                    tests[i].name, live_allocations);
            ok = 0;
        }
#endif
        printf("%s: %s\n", ok ? "PASS" : "FAIL", tests[i].name);
        passed += ok != 0;
    }
    size_t total = sizeof(tests) / sizeof(tests[0]);
    printf("Sequential buffer tests: %zu/%zu %s\n", passed, total,
           passed == total ? "PASS" : "FAIL");
    return passed == total ? EXIT_SUCCESS : EXIT_FAILURE;
}
