#ifndef OS_PC_BUFFER_H
#define OS_PC_BUFFER_H

#include <stddef.h>
#include "item.h"

struct buffer {
    struct item *items;
    size_t head;
    size_t tail;
    size_t count;
    size_t capacity;
};

enum buffer_status {
    BUFFER_OK = 0,
    BUFFER_INVALID_ARGUMENT,
    BUFFER_FULL,
    BUFFER_EMPTY,
    BUFFER_SIZE_OVERFLOW,
    BUFFER_NO_MEMORY,
    BUFFER_ALREADY_INITIALIZED
};

/* Start with struct buffer b = {0}; never copy a live owning buffer.
 * init allocates its array; errors leave the object unchanged.
 * Sequential API: callers supply all synchronization when sharing it.
 */
int buffer_init(struct buffer *buffer, size_t capacity);
/* Copies a DATA or STOP item by value; errors leave the queue unchanged. */
int buffer_push(struct buffer *buffer, struct item item);
/* out must point to a separate writable item, outside the buffer/its array.
 * Errors leave both the queue and *out unchanged.
 */
int buffer_pop(struct buffer *buffer, struct item *out);
/* Accepts NULL, zero, or live objects. Frees the array and resets to zero.
 * Safe to repeat; no threads may still be using the object.
 */
void buffer_destroy(struct buffer *buffer);

#endif
