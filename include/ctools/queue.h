#ifndef QUEUE_NAME
#error "QUEUE_NAME must be defined before including queue.h"
#endif

#ifndef QUEUE_TYPE
#error "QUEUE_TYPE must be defined before including queue.h"
#endif

#ifndef QUEUE_INDEX
#define QUEUE_INDEX unsigned int
#endif

#include "ctools/define_concat.h"
#include <stdbool.h>
#ifndef QUEUE_NO_IMPLEMENTATION
#include <errno.h>
#include <string.h>
#endif

#ifndef QUEUE_NO_INTERFACE

typedef struct QUEUE_NAME {
    QUEUE_TYPE *array;
    QUEUE_INDEX capacity_mask;

    QUEUE_INDEX front;
    QUEUE_INDEX back;
} QUEUE_NAME;

static const QUEUE_INDEX __EXPAND_CONCAT(QUEUE_NAME, _max_size) = (QUEUE_INDEX)1
                                                                  << (sizeof(QUEUE_INDEX) * 8 - 1 -
                                                                      !(((QUEUE_INDEX)-1) > 0));

static int __EXPAND_CONCAT(QUEUE_NAME, _create)(QUEUE_NAME *queue_dst,
                                                QUEUE_INDEX minimum_capacity);
static inline void __EXPAND_CONCAT(QUEUE_NAME, _destroy)(QUEUE_NAME *q);
static inline void __EXPAND_CONCAT(QUEUE_NAME, _peek)(QUEUE_NAME *q, QUEUE_TYPE *dst);
static inline QUEUE_INDEX __EXPAND_CONCAT(QUEUE_NAME, _size)(QUEUE_NAME *q);
static inline bool __EXPAND_CONCAT(QUEUE_NAME, _capacity)(QUEUE_NAME *q);
static inline bool __EXPAND_CONCAT(QUEUE_NAME, _is_full)(QUEUE_NAME *q);
static inline bool __EXPAND_CONCAT(QUEUE_NAME, _is_empty)(QUEUE_NAME *q);
static inline int __EXPAND_CONCAT(QUEUE_NAME, _push)(QUEUE_NAME *q, QUEUE_TYPE value);
static inline int __EXPAND_CONCAT(QUEUE_NAME, _pop)(QUEUE_NAME *q, QUEUE_TYPE *dst);

#endif // QUEUE_NO_INTERFACE

#ifndef QUEUE_NO_IMPLEMENTATION

static int __EXPAND_CONCAT(QUEUE_NAME, _create)(QUEUE_NAME *queue_dst,
                                                QUEUE_INDEX minimum_capacity) {
    // Skip if the requested size is not supported, given the QUEUE_INDEX type
    if (minimum_capacity > __EXPAND_CONCAT(QUEUE_NAME, _max_size) - 1)
        return EINVAL;

    // This implementation require one empty slot to distinguish between an empty queue and a full
    // queue.
    minimum_capacity += 1;

    // The actual queue object to allocate and initilize
    QUEUE_NAME q;

    // The actual capacity that the queue will have
    QUEUE_INDEX capacity = 1;

    // Assign the real capacity to the 2^n upper ceiling of the minimum capacity
    while (capacity < minimum_capacity)
        capacity <<= 1;

    // Allocate the container array
    q.array = (QUEUE_TYPE *)malloc(capacity * sizeof(QUEUE_TYPE));
    if (!q.array)
        return errno;

    // Initialize the rest of fields
    q.capacity_mask = capacity - 1;
    q.back = 0;
    q.front = 0;

    *queue_dst = q;

    return 0;
}

static inline void __EXPAND_CONCAT(QUEUE_NAME, _destroy)(QUEUE_NAME *q) { free(q->array); }

static inline void __EXPAND_CONCAT(QUEUE_NAME, _peek)(QUEUE_NAME *q, QUEUE_TYPE *dst) {
    *dst = q->array[q->front];
}

static inline QUEUE_INDEX __EXPAND_CONCAT(QUEUE_NAME, _size)(QUEUE_NAME *q) {
    QUEUE_INDEX back = q->back + (q->capacity_mask + 1) * (q->back < q->front);
    return back - q->front;
}

static inline bool __EXPAND_CONCAT(QUEUE_NAME, _capacity)(QUEUE_NAME *q) {
    return q->capacity_mask + 1;
}

static inline bool __EXPAND_CONCAT(QUEUE_NAME, _is_full)(QUEUE_NAME *q) {
    return q->front == ((q->back + 1) & q->capacity_mask);
}

static inline bool __EXPAND_CONCAT(QUEUE_NAME, _is_empty)(QUEUE_NAME *q) {
    return q->front == q->back;
}

static inline int __EXPAND_CONCAT(QUEUE_NAME, _push)(QUEUE_NAME *q, QUEUE_TYPE value) {
    // Skip if queue is full
    if (q->front == ((q->back + 1) & q->capacity_mask))
        return -1;

    q->array[q->back] = value;
    q->back = (q->back + 1) & q->capacity_mask;

    return 0;
}

static inline int __EXPAND_CONCAT(QUEUE_NAME, _pop)(QUEUE_NAME *q, QUEUE_TYPE *dst) {
    // Skip if queue is empty
    if (q->front == q->back)
        return -1;

    *dst = q->array[q->front];
    q->front = (q->front + 1) & q->capacity_mask;

    return 0;
}

#endif // QUEUE_NO_IMPLEMENTATION
