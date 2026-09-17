#ifndef TWHEEL_NAME
#error "TWHEEL_NAME must be defined before including TWHEEL_NAME.h"
#endif

#ifndef TWHEEL_TYPE
#error "TWHEEL_TYPE must be defined before including TWHEEL_NAME.h"
#endif

#ifndef TWHEEL_INDEX
#define TWHEEL_INDEX unsigned int
#endif

#ifndef TWHEEL_BUCKET_INDEX
#define TWHEEL_BUCKET_INDEX TWHEEL_INDEX
#endif

#ifndef TWHEEL_TICK
#define TWHEEL_TICK unsigned int
#endif

#include "ctools/define_concat.h"
#ifndef TWHEEL_NO_IMPLEMENTATION
#include <errno.h>
#include <stdlib.h>
#endif

#ifdef TWHEEL_NO_INTERFACE
#include "ctools/bitset_impl.h"
#else
#include "ctools/bitset.h"
#endif

#ifndef TWHEEL_NO_INTERFACE

struct __EXPAND_CONCAT(TWHEEL_NAME, _timer) {
    TWHEEL_INDEX next;
    TWHEEL_INDEX prev;
    TWHEEL_INDEX generation;
    TWHEEL_INDEX bucket;
};

struct __EXPAND_CONCAT(TWHEEL_NAME, _bucket) {
    TWHEEL_INDEX head;
    TWHEEL_INDEX tail;
};

struct __EXPAND_CONCAT(TWHEEL_NAME, _handle) {
    TWHEEL_INDEX timer_idx;
    TWHEEL_INDEX generation;
};

struct TWHEEL_NAME {
    TWHEEL_TICK interval;    // The amount of time between each bucket.
    TWHEEL_TICK time_modulo; // The amount of time progressed towards the next bucket.
    TWHEEL_TICK max_timeout; // The largest timeout possible given the constructor parameters.
    TWHEEL_INDEX capacity;   // The amount of timeouts available in this timer wheel.

    // The metadata of the timers.
    struct __EXPAND_CONCAT(TWHEEL_NAME, _timer) * timers;
    // The value to return to the caller if the timeout expires.
    TWHEEL_TYPE *return_values;

    // The timeout buckets
    struct __EXPAND_CONCAT(TWHEEL_NAME, _bucket) * buckets;
    TWHEEL_BUCKET_INDEX bucket_count;
    TWHEEL_BUCKET_INDEX current_bucket;

    // The stack of free data slots
    TWHEEL_INDEX *free_stack;
    TWHEEL_INDEX free_stack_head;

    // The expiration queue
    TWHEEL_INDEX exp_head;
    TWHEEL_INDEX exp_tail;

    //
    struct bitset occupied_buckets;
};

int __EXPAND_CONCAT(TWHEEL_NAME, _create)(struct TWHEEL_NAME *wheel, const TWHEEL_TICK interval,
                                          const unsigned int bucket_count,
                                          const TWHEEL_INDEX timeout_slot_capacity);
static inline void __EXPAND_CONCAT(TWHEEL_NAME, _destroy)(struct TWHEEL_NAME *tw);
static inline bool __EXPAND_CONCAT(TWHEEL_NAME, _is_full)(struct TWHEEL_NAME *tw);
static inline bool __EXPAND_CONCAT(TWHEEL_NAME, _is_empty)(struct TWHEEL_NAME *tw);
static inline int __EXPAND_CONCAT(TWHEEL_NAME, _advance)(struct TWHEEL_NAME *tw,
                                                         TWHEEL_TICK time_units);
static inline int
    __EXPAND_CONCAT(TWHEEL_NAME,
                    _schedule)(struct TWHEEL_NAME *tw, TWHEEL_TICK timeout, TWHEEL_TYPE value,
                               struct __EXPAND_CONCAT(TWHEEL_NAME, _handle) * timer_handle);
static inline int __EXPAND_CONCAT(TWHEEL_NAME,
                                  _cancel)(struct TWHEEL_NAME *tw,
                                           const struct __EXPAND_CONCAT(TWHEEL_NAME, _handle)
                                               timer_handle);
static inline int __EXPAND_CONCAT(TWHEEL_NAME, _pop)(struct TWHEEL_NAME *tw,
                                                     TWHEEL_TYPE *out_value);
static inline TWHEEL_TICK __EXPAND_CONCAT(TWHEEL_NAME, _wait)(struct TWHEEL_NAME *tw,
                                                              TWHEEL_TICK *ticks);

#endif // TWHEEL_NO_INTERFACE

#ifndef TWHEEL_NO_IMPLEMENTATION

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

int __EXPAND_CONCAT(TWHEEL_NAME, _create)(struct TWHEEL_NAME *wheel, const TWHEEL_TICK interval,
                                          const unsigned int bucket_count,
                                          const TWHEEL_INDEX timeout_slot_capacity) {
    // Verify parameter values
    int invalid_argument = interval < 1 || bucket_count < 1 || timeout_slot_capacity < 1;
    if (invalid_argument) {
        errno = EINVAL;
        return -1;
    }

    TWHEEL_INDEX *free_stack = (TWHEEL_INDEX *)malloc(timeout_slot_capacity * sizeof(TWHEEL_INDEX));
    if (!free_stack)
        return -1;

    struct __EXPAND_CONCAT(TWHEEL_NAME, _bucket) *buckets =
        (struct __EXPAND_CONCAT(TWHEEL_NAME, _bucket) *)malloc(
            bucket_count * sizeof(struct __EXPAND_CONCAT(TWHEEL_NAME, _bucket)));
    if (!buckets) {
        free(free_stack);
        return -1;
    }

    struct __EXPAND_CONCAT(TWHEEL_NAME, _timer) *timers =
        (struct __EXPAND_CONCAT(TWHEEL_NAME, _timer) *)malloc(
            timeout_slot_capacity * sizeof(struct __EXPAND_CONCAT(TWHEEL_NAME, _timer)));
    if (!timers) {
        free(buckets);
        free(free_stack);
        return -1;
    }

    TWHEEL_TYPE *return_values = (TWHEEL_TYPE *)malloc(timeout_slot_capacity * sizeof(TWHEEL_TYPE));
    if (!return_values) {
        free(timers);
        free(buckets);
        free(free_stack);
        return -1;
    }

    struct bitset occupied_buckets;
    if (bitset_create(&occupied_buckets, bucket_count)) {
        free(return_values);
        free(timers);
        free(buckets);
        free(free_stack);
        return -1;
    }

    for (TWHEEL_INDEX i = 0; i < timeout_slot_capacity; i++) {
        free_stack[i] = i;
        timers[i] = (struct __EXPAND_CONCAT(TWHEEL_NAME, _timer)){
            .next = timeout_slot_capacity,
            .prev = timeout_slot_capacity,
            .generation = 0,
            .bucket = bucket_count,
        };
    }

    for (TWHEEL_INDEX i = 0; i < bucket_count; i++) {
        buckets[i].head = timeout_slot_capacity;
        buckets[i].tail = timeout_slot_capacity;
    }

    const TWHEEL_TICK max_timeout = interval * bucket_count - 1;

    *wheel = (struct TWHEEL_NAME){
        .interval = interval,
        .time_modulo = 0,
        .max_timeout = max_timeout,
        .capacity = timeout_slot_capacity,
        .timers = timers,
        .return_values = return_values,
        .buckets = buckets,
        .bucket_count = bucket_count,
        .current_bucket = 0,
        .free_stack = free_stack,
        .free_stack_head = 0,
        .exp_head = timeout_slot_capacity,
        .exp_tail = timeout_slot_capacity,
        .occupied_buckets = occupied_buckets,
    };

    return 0;
}

static inline void __EXPAND_CONCAT(TWHEEL_NAME, _destroy)(struct TWHEEL_NAME *tw) {
    bitset_destroy(&tw->occupied_buckets);
    free(tw->return_values);
    free(tw->timers);
    free(tw->buckets);
    free(tw->free_stack);
}

static inline bool __EXPAND_CONCAT(TWHEEL_NAME, _is_full)(struct TWHEEL_NAME *tw) {
    return tw->free_stack_head >= tw->capacity;
}

static inline bool __EXPAND_CONCAT(TWHEEL_NAME, _is_empty)(struct TWHEEL_NAME *tw) {
    return tw->free_stack_head == 0;
}

static inline int __EXPAND_CONCAT(TWHEEL_NAME, _advance)(struct TWHEEL_NAME *tw,
                                                         TWHEEL_TICK time_units) {
    time_units = MIN(time_units, tw->max_timeout + 1);

    const TWHEEL_TICK time_passed_since_current_bucket = time_units + tw->time_modulo;
    const TWHEEL_BUCKET_INDEX buckets_expired = time_passed_since_current_bucket / tw->interval;
    const TWHEEL_BUCKET_INDEX next_current_bucket =
        (tw->current_bucket + buckets_expired) % tw->bucket_count;

    // The amount of buckets that pushed timers to the expired-queue
    TWHEEL_BUCKET_INDEX buckets_expired_count = 0;

    for (TWHEEL_BUCKET_INDEX bucket_idx = tw->current_bucket, first_iteration = 0;
         bucket_idx != next_current_bucket || !first_iteration;
         bucket_idx = (bucket_idx + 1) % tw->bucket_count, first_iteration++) {
        // Skip if the bucket is empty
        if (tw->buckets[bucket_idx].head == tw->capacity)
            continue;

        // Splice the bucket to the end of the expired-queue
        if (tw->exp_head == tw->capacity) {
            tw->exp_head = tw->buckets[bucket_idx].head;
            tw->exp_tail = tw->buckets[bucket_idx].tail; // expired_timers_tail = bucket_tail
        } else {
            tw->timers[tw->buckets[bucket_idx].head].prev =
                tw->exp_tail; // bucket_head.prev = expired_timers_tail
            tw->timers[tw->exp_tail].next =
                tw->buckets[bucket_idx].head; // expired_timers_tail.next = bucket_head

            tw->exp_tail = tw->buckets[bucket_idx].tail; // expired_timers_tail = bucket_tail
        }

        // Reset the bucket
        tw->buckets[bucket_idx].head = tw->capacity;
        tw->buckets[bucket_idx].tail = tw->capacity;
        bitset_assign(&tw->occupied_buckets, bucket_idx, 0); // Mark the bucket as unoccupied

        // Increment the counter on expired buckets
        buckets_expired_count++;
    }

    // Update the current bucket
    tw->current_bucket = next_current_bucket;

    // Update the remaining time
    tw->time_modulo = time_passed_since_current_bucket % tw->interval;

    return buckets_expired_count;
}

static inline int
__EXPAND_CONCAT(TWHEEL_NAME,
                _schedule)(struct TWHEEL_NAME *tw, TWHEEL_TICK timeout, TWHEEL_TYPE value,
                           struct __EXPAND_CONCAT(TWHEEL_NAME, _handle) * timer_handle) {
    if (__EXPAND_CONCAT(TWHEEL_NAME, _is_full)(tw)) {
        errno = ENOBUFS;
        return -1;
    }

    // Disallow timeouts beyond the wheel's size
    if (timeout > tw->max_timeout) {
        errno = EINVAL;
        return -1;
    }

    // Disallow timeouts less than 0 ticks
    if (timeout < 1) {
        errno = EINVAL;
        return -1;
    }

    // Pop an available timer
    const TWHEEL_INDEX new_timer_idx = tw->free_stack[tw->free_stack_head++];

    // Assign the return value to the new timer
    tw->return_values[new_timer_idx] = value;

    // Select the correct bucket
    const TWHEEL_INDEX bucket_overflow =
        tw->current_bucket + (timeout + tw->time_modulo) / tw->interval;
    const TWHEEL_INDEX bucket =
        bucket_overflow - tw->bucket_count * (bucket_overflow >= tw->bucket_count);

    // Add the timeout timer to the correct bucket
    tw->timers[new_timer_idx].next =
        tw->capacity; // No timers after this one, since it will be placed at the tail.
    tw->timers[new_timer_idx].prev = tw->buckets[bucket].tail;
    tw->timers[new_timer_idx].generation = tw->timers[new_timer_idx].generation;
    tw->timers[new_timer_idx].bucket = bucket;
    if (tw->buckets[bucket].tail != tw->capacity)
        tw->timers[tw->buckets[bucket].tail].next = new_timer_idx;
    tw->buckets[bucket].tail = new_timer_idx;

    // In case this is the first timer
    if (tw->buckets[bucket].head == tw->capacity) {
        tw->buckets[bucket].head = new_timer_idx;        // Update the head
        bitset_assign(&tw->occupied_buckets, bucket, 1); // Mark the bucket as occupied
    }

    // Give the caller a handle to the new timer, so that it can be canceled.
    if (timer_handle) {
        timer_handle->timer_idx = new_timer_idx;
        timer_handle->generation = tw->timers[new_timer_idx].generation;
    }

    return 0;
}

static inline int __EXPAND_CONCAT(TWHEEL_NAME,
                                  _cancel)(struct TWHEEL_NAME *tw,
                                           const struct __EXPAND_CONCAT(TWHEEL_NAME, _handle)
                                               timer_handle) {
    const TWHEEL_BUCKET_INDEX bucket = tw->timers[timer_handle.timer_idx].bucket;

    // Remove the target entry from the linked list
    const TWHEEL_INDEX next = tw->timers[timer_handle.timer_idx].next;
    const TWHEEL_INDEX prev = tw->timers[timer_handle.timer_idx].prev;
    const bool is_first_element = prev == tw->capacity;
    const bool is_final_element = next == tw->capacity;

    // Remove pointers from the timers
    if (!is_first_element)
        tw->timers[prev].next = next;
    if (!is_final_element)
        tw->timers[next].prev = prev;

    // Remove pointers from the bucket
    if (is_first_element)
        tw->buckets[bucket].head = next;
    if (is_final_element)
        tw->buckets[bucket].tail = prev;

    // Return the timer to the free stack
    tw->free_stack[--tw->free_stack_head] = timer_handle.timer_idx;

    // If this was the last timer in the bucket, reset the bucket
    if (is_first_element & is_final_element) {
        tw->buckets[bucket].head = tw->capacity;
        tw->buckets[bucket].tail = tw->capacity;
        bitset_assign(&tw->occupied_buckets, bucket, 0); // Mark the bucket as unoccupied
    }

    return 0;
}

static inline int __EXPAND_CONCAT(TWHEEL_NAME, _pop)(struct TWHEEL_NAME *tw,
                                                     TWHEEL_TYPE *out_value) {
    if (tw->exp_head == tw->capacity) {
        errno = ENOBUFS;
        return -1;
    }

    // Return the value to the caller
    *out_value = tw->return_values[tw->exp_head];

    // Return the timer to the free-queue
    tw->free_stack[--tw->free_stack_head] = tw->exp_head;

    // Go to the next timer in the expired-queue
    tw->exp_head = tw->timers[tw->exp_head].next;

    // Reset the expired-queue if this was the final timer
    if (tw->exp_head == tw->capacity)
        tw->exp_tail = tw->capacity;

    return 0;
}

static inline TWHEEL_TICK __EXPAND_CONCAT(TWHEEL_NAME, _wait)(struct TWHEEL_NAME *tw,
                                                              TWHEEL_TICK *ticks) {
    if (__EXPAND_CONCAT(TWHEEL_NAME, _is_empty)(tw))
        return 1;

    TWHEEL_BUCKET_INDEX next_bucket = tw->bucket_count;

    // Find the next bucket
    int err = bitset_search_up(&tw->occupied_buckets, &next_bucket, tw->current_bucket + 1,
                               tw->bucket_count);
    if (err)
        err = bitset_search_up(&tw->occupied_buckets, &next_bucket, 0, tw->current_bucket);

    // If we found a bucket, return the time to the next timeout
    if (!err) {
        const TWHEEL_BUCKET_INDEX bucket_distance =
            next_bucket > tw->current_bucket ? next_bucket - tw->current_bucket
                                             : tw->bucket_count - tw->current_bucket + next_bucket;

        *ticks = bucket_distance * tw->interval - tw->time_modulo;
    }

    return err;
}

#undef MIN
#endif // TWHEEL_NO_IMPLEMENTATION
