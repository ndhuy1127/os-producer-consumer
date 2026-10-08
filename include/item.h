#ifndef OS_PC_ITEM_H
#define OS_PC_ITEM_H

#include <stdint.h>

enum item_kind { ITEM_DATA, ITEM_STOP };

struct item {
    enum item_kind kind;
    uint64_t id; /* Meaningful as a data ID only for ITEM_DATA. */
    int value;
};

#endif
