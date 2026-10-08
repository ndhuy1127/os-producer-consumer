#include "buffer.h"

#include <stdint.h>
#include <stdlib.h>

static int is_live(const struct buffer *buffer)
{
    return buffer != NULL && buffer->items != NULL &&
           buffer->capacity > 0 && buffer->head < buffer->capacity &&
           buffer->tail < buffer->capacity && buffer->count <= buffer->capacity;
}

int buffer_init(struct buffer *buffer, size_t capacity)
{
    if (buffer == NULL || capacity == 0) {
        return BUFFER_INVALID_ARGUMENT;
    }
    if (buffer->items != NULL || buffer->head != 0 || buffer->tail != 0 ||
        buffer->count != 0 || buffer->capacity != 0) {
        return BUFFER_ALREADY_INITIALIZED;
    }
    if (capacity > SIZE_MAX / sizeof(struct item)) {
        return BUFFER_SIZE_OVERFLOW;
    }

    struct item *items = malloc(capacity * sizeof(*items));
    if (items == NULL) {
        return BUFFER_NO_MEMORY;
    }

    buffer->items = items;
    buffer->capacity = capacity;
    return BUFFER_OK;
}

int buffer_push(struct buffer *buffer, struct item item)
{
    if (!is_live(buffer) ||
        (item.kind != ITEM_DATA && item.kind != ITEM_STOP)) {
        return BUFFER_INVALID_ARGUMENT;
    }
    if (buffer->count == buffer->capacity) {
        return BUFFER_FULL;
    }

    buffer->items[buffer->tail] = item;
    buffer->tail = (buffer->tail + 1) % buffer->capacity;
    ++buffer->count;
    return BUFFER_OK;
}

int buffer_pop(struct buffer *buffer, struct item *out)
{
    if (!is_live(buffer) || out == NULL) {
        return BUFFER_INVALID_ARGUMENT;
    }
    if (buffer->count == 0) {
        return BUFFER_EMPTY;
    }

    *out = buffer->items[buffer->head];
    buffer->head = (buffer->head + 1) % buffer->capacity;
    --buffer->count;
    return BUFFER_OK;
}

void buffer_destroy(struct buffer *buffer)
{
    if (buffer != NULL) {
        free(buffer->items);
        *buffer = (struct buffer){0};
    }
}
