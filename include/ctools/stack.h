#ifndef STACK_TYPE
#error "STACK_TYPE must be defined before including stack.h"
#endif

#ifndef STACK_NAME
#error "STACK_NAME must be defined before including stack.h"
#endif

#ifndef STACK_INDEX
#define STACK_INDEX unsigned int
#endif

#ifndef STACK_MIN_CAPACITY
#define STACK_MIN_CAPACITY 8
#endif

#ifdef STACK_CAPACITY
#ifdef STACK_EXT_DYNAMIC_SIZE
#error "STACK_CAPACITY and STACK_EXT_DYNAMIC_SIZE are incompatible"
#endif
#endif

#include "ctools/define_concat.h"
#include <stdbool.h>
#ifndef STACK_NO_IMPLEMENTATION
#include <errno.h>
#include <stdlib.h>
#endif
#ifdef STACK_EXT_THREAD_SAFE
#include <pthread.h>
#endif

#ifndef STACK_NO_INTERFACE

struct STACK_NAME {
    STACK_INDEX size;

#ifdef STACK_CAPACITY
    STACK_TYPE array[STACK_CAPACITY];
#else
    STACK_INDEX capacity;
    STACK_TYPE *array;
#endif

#ifdef STACK_EXT_THREAD_SAFE
    pthread_mutex_t lock;
    bool shutting_down;
#endif
};

static inline int __EXPAND_CONCAT(STACK_NAME, _create)(struct STACK_NAME *s
#ifndef STACK_CAPACITY
                                                       ,
#ifdef STACK_EXT_DYNAMIC_SIZE
                                                       const STACK_INDEX initial_capacity
#else
                                                       const STACK_INDEX capacity
#endif // STACK_EXT_DYNAMIC_SIZE
#endif // STACK_CAPACITY
);

static inline void __EXPAND_CONCAT(STACK_NAME, _shutdown)(struct STACK_NAME *s);
static inline void __EXPAND_CONCAT(STACK_NAME, _destroy)(struct STACK_NAME *s);
static inline int __EXPAND_CONCAT(STACK_NAME, _push)(struct STACK_NAME *s, STACK_TYPE value);
static inline int __EXPAND_CONCAT(STACK_NAME, _pop)(struct STACK_NAME *s, STACK_TYPE *dst);
static inline int __EXPAND_CONCAT(STACK_NAME, _clear)(struct STACK_NAME *s);

static inline STACK_INDEX __EXPAND_CONCAT(STACK_NAME, _size)
#ifndef STACK_EXT_THREAD_SAFE
    (const struct STACK_NAME *s);
#else
    (struct STACK_NAME *s);
#endif // STACK_EXT_THREAD_SAFE

#endif // STACK_NO_INTERFACE

#ifndef STACK_NO_IMPLEMENTATION

static inline int __EXPAND_CONCAT(STACK_NAME, _create)(struct STACK_NAME *s
#ifndef STACK_CAPACITY
                                                       ,
#ifdef STACK_EXT_DYNAMIC_SIZE
                                                       const STACK_INDEX initial_capacity
#else
                                                       const STACK_INDEX capacity
#endif // STACK_EXT_DYNAMIC_SIZE
#endif // STACK_CAPACITY
) {
#ifdef STACK_EXT_THREAD_SAFE
    if (pthread_mutex_init(&s->lock, NULL)) {
        return -1;
    }
#endif

#ifndef STACK_CAPACITY

#ifdef STACK_EXT_DYNAMIC_SIZE
    // Choose the largest of `initial_capacity` and `STACK_MIN_CAPACITY` as the starting capacity.
    const STACK_INDEX capacity =
        initial_capacity > STACK_MIN_CAPACITY ? initial_capacity : STACK_MIN_CAPACITY;
#endif // STACK_EXT_DYNAMIC_SIZE

    s->array = (STACK_TYPE *)malloc(capacity * sizeof(STACK_TYPE));
    if (!s->array) {
#ifdef STACK_EXT_THREAD_SAFE
        pthread_mutex_destroy(&s->lock);
#endif
        return -1;
    }

    s->capacity = capacity;
#endif

    s->size = 0;

#ifdef STACK_EXT_THREAD_SAFE
    s->shutting_down = false;
#endif

    return 0;
}

static inline void __EXPAND_CONCAT(STACK_NAME, _shutdown)(struct STACK_NAME *s) {
#ifdef STACK_EXT_THREAD_SAFE

    // Lock
    pthread_mutex_lock(&s->lock);

    s->shutting_down = true;

    // Unlock
    pthread_mutex_unlock(&s->lock);

#endif
}

static inline void __EXPAND_CONCAT(STACK_NAME, _destroy)(struct STACK_NAME *s) {
// Aquire lock
#ifdef STACK_EXT_THREAD_SAFE
    pthread_mutex_lock(&s->lock);
#endif

// Destroy all nodes in the stack
#ifndef STACK_CAPACITY
    free(s->array);
#endif

#ifdef STACK_EXT_THREAD_SAFE

    // Unlock
    pthread_mutex_unlock(&s->lock);

    // Destroy the lock
    pthread_mutex_destroy(&s->lock);

#endif
}

static inline STACK_INDEX __EXPAND_CONCAT(STACK_NAME, _size)
#ifndef STACK_EXT_THREAD_SAFE
    (const struct STACK_NAME *s)
#else
    (struct STACK_NAME *s)
#endif // STACK_EXT_THREAD_SAFE
{
#ifdef STACK_EXT_THREAD_SAFE

    pthread_mutex_lock(&s->lock);

    STACK_INDEX size = s->size;

    pthread_mutex_unlock(&s->lock);

    return size;

#else
    return s->size;
#endif
}

static inline int __EXPAND_CONCAT(STACK_NAME, _push)(struct STACK_NAME *s, STACK_TYPE value) {
#ifdef STACK_EXT_THREAD_SAFE

    // Lock
    pthread_mutex_lock(&s->lock);

    // Cancel if the stack is shutting down
    if (s->shutting_down) {
        pthread_mutex_unlock(&s->lock);
        errno = ECANCELED;
        return -1;
    }

#endif // STACK_EXT_THREAD_SAFE
#ifdef STACK_EXT_DYNAMIC_SIZE

    // Increase capacity if the stack is full
    if (s->size == s->capacity) {
        STACK_TYPE *new_array =
            (STACK_TYPE *)realloc(s->array, 2 * s->capacity * sizeof(STACK_TYPE));

        // If we failed to allocate more memory, return an error
        if (!new_array) {
#ifdef STACK_EXT_THREAD_SAFE
            pthread_mutex_unlock(&s->lock);
#endif
            return -1;
        }

        // Continue with the reallocated array
        s->array = new_array;
        s->capacity *= 2;
    }

#else

    if (s->size == s->capacity) {
        errno = ENOBUFS;
        return -1;
    }

#endif // STACK_EXT_DYNAMIC_SIZE

    // Assign the value to the top of the stack,
    // then increase the size counter
    s->array[s->size++] = value;

// Unlock
#ifdef STACK_EXT_THREAD_SAFE
    pthread_mutex_unlock(&s->lock);
#endif

    return 0;
}

static inline int __EXPAND_CONCAT(STACK_NAME, _pop)(struct STACK_NAME *s, STACK_TYPE *dst) {
#ifdef STACK_EXT_THREAD_SAFE

    // Lock
    pthread_mutex_lock(&s->lock);

    // If the stack is shutting down, skip
    if (s->shutting_down) {
        pthread_mutex_unlock(&s->lock);
        errno = ECANCELED;
        return -1;
    }

#endif

    // Return error code if the stack is empty
    if (s->size == 0) {
#ifdef STACK_EXT_THREAD_SAFE
        pthread_mutex_unlock(&s->lock);
#endif

        errno = ENOENT;
        return -1;
    }

    // Decrease size counter by one,
    // then assign the removed value to the destination pointer
    *dst = s->array[--s->size];

#ifdef STACK_EXT_DYNAMIC_SIZE

    // Decrease capacity if the stack size is a quarter of the capacity
    if ((s->size <= s->capacity / 4) && (s->capacity / 2 >= STACK_MIN_CAPACITY)) {
        STACK_TYPE *new_array =
            (STACK_TYPE *)realloc(s->array, (s->capacity / 2) * sizeof(STACK_TYPE));

        // If we failed to allocate more memory, return an error
        if (!new_array) {
#ifdef STACK_EXT_THREAD_SAFE
            pthread_mutex_unlock(&s->lock);
#endif
            return -1;
        }

        s->array = new_array;
        s->capacity /= 2;
    }

#endif // STACK_EXT_DYNAMIC_SIZE

// Unlock
#ifdef STACK_EXT_THREAD_SAFE
    pthread_mutex_unlock(&s->lock);
#endif

    // Return success
    return 0;
}

static inline int __EXPAND_CONCAT(STACK_NAME, _clear)(struct STACK_NAME *s) {
#ifdef STACK_EXT_THREAD_SAFE

    // Lock
    pthread_mutex_lock(&s->lock);

    // If the stack is shutting down, skip
    if (s->shutting_down) {
        pthread_mutex_unlock(&s->lock);
        errno = ECANCELED;
        return -1;
    }

#endif

    // Reset the size counter
    s->size = 0;

#ifdef STACK_EXT_DYNAMIC_SIZE

    // Decrease capacity if the stack size is a quarter of the capacity
    if ((s->size <= s->capacity / 4) && (s->capacity / 2 >= STACK_MIN_CAPACITY)) {
        STACK_TYPE *new_array =
            (STACK_TYPE *)realloc(s->array, (s->capacity / 2) * sizeof(STACK_TYPE));

        // If we failed to allocate more memory, return an error
        if (!new_array) {
#ifdef STACK_EXT_THREAD_SAFE
            pthread_mutex_unlock(&s->lock);
#endif
            return -1;
        }

        s->array = new_array;
        s->capacity /= 2;
    }

#endif // STACK_EXT_DYNAMIC_SIZE

// Unlock
#ifdef STACK_EXT_THREAD_SAFE
    pthread_mutex_unlock(&s->lock);
#endif

    // Return success
    return 0;
}

#endif // STACK_NO_IMPLEMENTATION
