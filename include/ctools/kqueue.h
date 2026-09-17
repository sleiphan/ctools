#ifndef KQUEUE_NAME
#error "KQUEUE_NAME must be defined before including kqueue.h"
#endif

#ifndef KQUEUE_TYPE
#error "KQUEUE_TYPE must be defined before including kqueue.h"
#endif

#ifndef KQUEUE_INDEX
#define KQUEUE_INDEX unsigned int
#endif

#ifndef KQUEUE_SUBQUEUE_INDEX
#define KQUEUE_SUBQUEUE_INDEX KQUEUE_INDEX
#endif

#include "ctools/define_concat.h"
#include <stdbool.h>
#ifndef KQUEUE_NO_IMPLEMENTATION
#include <errno.h>
#include <stdlib.h>
#endif

#ifndef KQUEUE_NO_INTERFACE

struct __EXPAND_CONCAT(KQUEUE_NAME, _subqueue) {
    KQUEUE_INDEX head;
    KQUEUE_INDEX tail;
};

struct KQUEUE_NAME {
    KQUEUE_TYPE *array;
    KQUEUE_INDEX *nexts;
    KQUEUE_INDEX capacity;

    KQUEUE_INDEX *free_stack;
    KQUEUE_INDEX free_stack_head;

    struct __EXPAND_CONCAT(KQUEUE_NAME, _subqueue) * queues;
    KQUEUE_SUBQUEUE_INDEX queue_count;
};

static const KQUEUE_INDEX __EXPAND_CONCAT(KQUEUE_NAME, _max_size) =
    ((KQUEUE_INDEX)-1) ^ ((((KQUEUE_INDEX)-1) < 0) << (sizeof(KQUEUE_INDEX) * 8 - 1));

static int __EXPAND_CONCAT(KQUEUE_NAME, _create)(struct KQUEUE_NAME *queue_dst,
                                                 const KQUEUE_INDEX capacity,
                                                 const KQUEUE_SUBQUEUE_INDEX queue_count);
static inline void __EXPAND_CONCAT(KQUEUE_NAME, _destroy)(struct KQUEUE_NAME *q);
static inline KQUEUE_INDEX __EXPAND_CONCAT(KQUEUE_NAME, _size)(struct KQUEUE_NAME *q);
static inline KQUEUE_INDEX __EXPAND_CONCAT(KQUEUE_NAME, _capacity)(struct KQUEUE_NAME *q);
static inline bool __EXPAND_CONCAT(KQUEUE_NAME, _is_full)(struct KQUEUE_NAME *q);
static inline bool __EXPAND_CONCAT(KQUEUE_NAME, _is_empty)(struct KQUEUE_NAME *q);
static inline int __EXPAND_CONCAT(KQUEUE_NAME, _peek)(struct KQUEUE_NAME *q,
                                                      KQUEUE_SUBQUEUE_INDEX queue_idx,
                                                      KQUEUE_TYPE *dst);
static int __EXPAND_CONCAT(KQUEUE_NAME, _push)(struct KQUEUE_NAME *q,
                                               const KQUEUE_SUBQUEUE_INDEX queue_idx,
                                               const KQUEUE_TYPE value);
static int __EXPAND_CONCAT(KQUEUE_NAME, _pop)(struct KQUEUE_NAME *q,
                                              const KQUEUE_SUBQUEUE_INDEX queue_idx,
                                              KQUEUE_TYPE *dst);

#endif // KQUEUE_NO_INTERFACE

#ifndef KQUEUE_NO_IMPLEMENTATION

static int __EXPAND_CONCAT(KQUEUE_NAME, _create)(struct KQUEUE_NAME *queue_dst,
                                                 const KQUEUE_INDEX capacity,
                                                 const KQUEUE_SUBQUEUE_INDEX queue_count) {
    // Allocate the container array
    KQUEUE_TYPE *array = (KQUEUE_TYPE *)malloc(capacity * sizeof(KQUEUE_TYPE));
    if (!array)
        return -1;

    // Allocate the queues array
    struct __EXPAND_CONCAT(KQUEUE_NAME, _subqueue) *queues =
        (struct __EXPAND_CONCAT(KQUEUE_NAME, _subqueue) *)malloc(
            queue_count * sizeof(struct __EXPAND_CONCAT(KQUEUE_NAME, _subqueue)));
    if (!queues) {
        free(array);
        return -1;
    }

    // Allocate nexts
    KQUEUE_INDEX *nexts = (KQUEUE_INDEX *)malloc(capacity * sizeof(KQUEUE_INDEX));
    if (!nexts) {
        free(queues);
        free(array);
        return -1;
    }

    // Allocate the free stack
    KQUEUE_INDEX *free_stack = (KQUEUE_INDEX *)malloc(capacity * sizeof(KQUEUE_INDEX));
    if (!free_stack) {
        free(queues);
        free(array);
        free(nexts);
        return -1;
    }

    // Initialize the nexts array
    for (KQUEUE_INDEX i = 0; i < capacity; i++)
        nexts[i] = __EXPAND_CONCAT(KQUEUE_NAME, _max_size);

    // Initialize the nexts stack
    for (KQUEUE_INDEX i = 0; i < capacity; i++)
        free_stack[i] = i;

    // Initialize all queues
    for (KQUEUE_SUBQUEUE_INDEX i = 0; i < queue_count; i++)
        queues[i] = (struct __EXPAND_CONCAT(KQUEUE_NAME, _subqueue)){
            .head = __EXPAND_CONCAT(KQUEUE_NAME, _max_size),
            .tail = __EXPAND_CONCAT(KQUEUE_NAME, _max_size),
        };

    // Create and return the actual object
    *queue_dst = (struct KQUEUE_NAME){
        .array = array,
        .nexts = nexts,
        .capacity = capacity,
        .free_stack = free_stack,
        .free_stack_head = 0,
        .queues = queues,
        .queue_count = queue_count,
    };

    return 0;
}

static inline void __EXPAND_CONCAT(KQUEUE_NAME, _destroy)(struct KQUEUE_NAME *q) {
    free(q->array);
    free(q->free_stack);
    free(q->nexts);
    free(q->queues);
}

static inline KQUEUE_INDEX __EXPAND_CONCAT(KQUEUE_NAME, _size)(struct KQUEUE_NAME *q) {
    return q->free_stack_head;
}

static inline KQUEUE_INDEX __EXPAND_CONCAT(KQUEUE_NAME, _capacity)(struct KQUEUE_NAME *q) {
    return q->capacity;
}

static inline bool __EXPAND_CONCAT(KQUEUE_NAME, _is_full)(struct KQUEUE_NAME *q) {
    return q->free_stack_head >= q->capacity;
}

static inline bool __EXPAND_CONCAT(KQUEUE_NAME, _is_empty)(struct KQUEUE_NAME *q) {
    return !q->free_stack_head;
}

static inline int __EXPAND_CONCAT(KQUEUE_NAME, _peek)(struct KQUEUE_NAME *q,
                                                      KQUEUE_SUBQUEUE_INDEX queue_idx,
                                                      KQUEUE_TYPE *dst) {
    // Skip if queue is empty
    if (__EXPAND_CONCAT(KQUEUE_NAME, _is_empty(q))) {
        errno = ENOENT;
        return -1;
    }

    *dst = q->array[q->queues[queue_idx].head];
    return 0;
}

static int __EXPAND_CONCAT(KQUEUE_NAME, _push)(struct KQUEUE_NAME *q,
                                               const KQUEUE_SUBQUEUE_INDEX queue_idx,
                                               const KQUEUE_TYPE value) {
    // Skip if queue is full
    if (__EXPAND_CONCAT(KQUEUE_NAME, _is_full(q))) {
        errno = ENOBUFS;
        return -1;
    }

    // Error if the queue_idx is invalid
    if (queue_idx >= q->queue_count) {
        errno = EINVAL;
        return -1;
    }

    // Pop new entry from the free stack
    const KQUEUE_INDEX new_entry_idx = q->free_stack[q->free_stack_head++];

    // Assign the submitted value to the new element
    q->array[new_entry_idx] = value;

    // Set the next-value of the tail entry to point to the new entry
    if (q->queues[queue_idx].tail != __EXPAND_CONCAT(KQUEUE_NAME, _max_size))
        q->nexts[q->queues[queue_idx].tail] = new_entry_idx;

    // Update the tail
    q->queues[queue_idx].tail = new_entry_idx;

    // If this is the first entry in this queue ...
    if (q->queues[queue_idx].head == __EXPAND_CONCAT(KQUEUE_NAME, _max_size))
        q->queues[queue_idx].head = new_entry_idx; // ... set the head to the new entry

    return 0;
}

static int __EXPAND_CONCAT(KQUEUE_NAME, _pop)(struct KQUEUE_NAME *q,
                                              const KQUEUE_SUBQUEUE_INDEX queue_idx,
                                              KQUEUE_TYPE *dst) {
    const KQUEUE_INDEX head = q->queues[queue_idx].head;

    // Skip if sub-queue is empty
    if (head == __EXPAND_CONCAT(KQUEUE_NAME, _max_size)) {
        errno = ENOENT;
        return -1;
    }

    // Return the value to the caller
    *dst = q->array[head];

    // Return the entry at the head back to the stack
    q->free_stack[--q->free_stack_head] = head;

    // If this is the last element in the given queue ...
    if (head == q->queues[queue_idx].tail) {
        // ... reset the given queue
        q->queues[queue_idx] = (struct __EXPAND_CONCAT(KQUEUE_NAME, _subqueue)){
            .head = __EXPAND_CONCAT(KQUEUE_NAME, _max_size),
            .tail = __EXPAND_CONCAT(KQUEUE_NAME, _max_size),
        };
    } else {
        // Else, forward the head
        q->queues[queue_idx].head = q->nexts[head];
    }

    return 0;
}

#endif // KQUEUE_NO_IMPLEMENTATION
