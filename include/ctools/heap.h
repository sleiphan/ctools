#ifndef HEAP_TYPE
#error "HEAP_TYPE must be defined before including heap.h"
#endif

#ifndef HEAP_NAME
#error "HEAP_NAME must be defined before including heap.h"
#endif

#ifndef HEAP_COMP
#error "HEAP_COMP must be defined before including heap.h"
#endif

#ifndef HEAP_INDEX
#define HEAP_INDEX unsigned int
#endif

#include "ctools/define_concat.h"

#ifndef HEAP_NO_IMPLEMENTATION
#include <stdlib.h>

#define STACK_NAME __EXPAND_CONCAT(HEAP_NAME, _stack)
#define STACK_TYPE HEAP_INDEX
#include "ctools/stack.h"

#ifdef CTOOLS_ENABLE_DEBUG_ASSERT
#include <assert.h>
#endif
#endif

#ifndef HEAP_NO_INTERFACE

typedef struct HEAP_NAME {
    HEAP_INDEX size;
    HEAP_INDEX capacity;
    HEAP_TYPE *array;
} HEAP_NAME;

/**
 * @brief Allocates and initializes a new heap.
 * @param initial_capacity The initial capacity of the internal storage array that contains the
 * nodes of the heap. Must be greater than 0.
 * @return A pointer to the newly allocated and initialized heap object.
 */
static inline HEAP_NAME *__EXPAND_CONCAT(HEAP_NAME, _create)(const HEAP_INDEX initial_capacity);

/**
 * @brief De-allocates a heap.
 * @param h A pointer to a heap allocated by the _create() function.
 */
static inline void __EXPAND_CONCAT(HEAP_NAME, _destroy)(HEAP_NAME *h);
static inline int __EXPAND_CONCAT(HEAP_NAME, _push)(HEAP_NAME *h, HEAP_TYPE value);
static inline int __EXPAND_CONCAT(HEAP_NAME, _pop)(HEAP_NAME *h, HEAP_TYPE *dst);
static inline HEAP_INDEX __EXPAND_CONCAT(HEAP_NAME, _size)(HEAP_NAME *h);
static inline HEAP_TYPE __EXPAND_CONCAT(HEAP_NAME, _peek)(HEAP_NAME *h);
static inline int __EXPAND_CONCAT(HEAP_NAME, _build)(HEAP_TYPE *heap_array,
                                                     const HEAP_INDEX heap_array_size);
static inline HEAP_INDEX __EXPAND_CONCAT(HEAP_NAME, _verify)(const HEAP_TYPE *heap_array,
                                                             const HEAP_INDEX heap_array_size);

#endif // HEAP_NO_INTERFACE

#ifndef HEAP_NO_IMPLEMENTATION

#ifndef HEAP_SWAP
#define HEAP_SWAP(LHS, RHS)                                                                        \
    {                                                                                              \
        HEAP_TYPE temp = LHS;                                                                      \
        LHS = RHS;                                                                                 \
        RHS = temp;                                                                                \
    }
#endif

static inline HEAP_NAME *__EXPAND_CONCAT(HEAP_NAME, _create)(const HEAP_INDEX initial_capacity) {
    // Allocate a new heap struct
    HEAP_NAME *h = (HEAP_NAME *)malloc(sizeof(HEAP_NAME));

    if (!h)
        return NULL;

    // Choose the largest of `initial_capacity` and `STACK_MIN_CAPACITY` as the starting capacity.
    const HEAP_INDEX starting_capacity =
        initial_capacity > STACK_MIN_CAPACITY ? initial_capacity : STACK_MIN_CAPACITY;

    // Assign default values
    h->size = 0;
    h->capacity = starting_capacity;

    // Allocate the array containing the heap nodes
    h->array = (HEAP_TYPE *)malloc(sizeof(HEAP_TYPE) * starting_capacity);

    if (!h->array) {
        free(h);
        return NULL;
    }

    // Return the new heap
    return h;
}

static inline void __EXPAND_CONCAT(HEAP_NAME, _destroy)(HEAP_NAME *h) {
    free(h->array);
    free(h);
}

static inline int __EXPAND_CONCAT(HEAP_NAME, _push)(HEAP_NAME *h, HEAP_TYPE value) {
    // If the storage array is full, double the capacity of the storage array
    if (h->size == h->capacity) {
        HEAP_TYPE *new_array = (HEAP_TYPE *)realloc(h->array, 2 * h->capacity * sizeof(HEAP_TYPE));

        // If we failed to allocate more memory, return an error
        if (!new_array) {
            return -3;
        }

        // Continue with the reallocated array
        h->array = new_array;
        h->capacity *= 2;
    }

    // The index of the last node in the storage array
    const HEAP_INDEX last_node = h->size;

    // Increase element count
    h->size++;

    // Assign the new value to the end of the heap
    h->array[last_node] = value;

    // Define the parent/child pair, with the new node as the child
    HEAP_INDEX parent_node = (last_node - 1) / 2;
    HEAP_INDEX child_node = last_node;

    // Move the child node upwards until the value of the parent node is larger than or equal to the
    // value of the child node
    while (child_node > 0 && HEAP_COMP(h->array[child_node], h->array[parent_node]) < 0) {

        // Swap parent and child
        HEAP_SWAP(h->array[child_node], h->array[parent_node])

        // Iterate to the above parent/child pair
        child_node = parent_node;
        parent_node = (parent_node - 1) / 2;
    }

    return 0;
}

static inline int __EXPAND_CONCAT(HEAP_NAME, _pop)(HEAP_NAME *h, HEAP_TYPE *dst) {
    // The node to pop is the root node
    *dst = h->array[0];

    // Overwrite the root node with the last element in the heap,
    // then decrease the size counter of the heap
    h->array[0] = h->array[--h->size];

    // Decrease capacity if the heap size is a quarter of the capacity
    if ((h->size <= h->capacity / 4) && (h->capacity / 2 >= STACK_MIN_CAPACITY)) {
        HEAP_TYPE *new_array =
            (HEAP_TYPE *)realloc(h->array, (h->capacity / 2) * sizeof(HEAP_TYPE));

        // If we failed to allocate more memory, return an error
        if (!new_array) {
            return -3;
        }

        h->array = new_array;
        h->capacity /= 2;
    }

    HEAP_INDEX parent = 0;

    // While there are nodes left to process
    while (1) {

        // Find the index of both child nodes
        const HEAP_INDEX left_child = (parent + 1) * 2 - 1;
        const HEAP_INDEX right_child = (parent + 1) * 2;

        const int parent_has_children = left_child < h->size;
        const int parent_has_two_children = right_child < h->size;

        // Cancel if the node has no children
        if (!parent_has_children)
            break;

        // Find the largest child
        HEAP_INDEX largest_child = left_child;
        if (parent_has_two_children) {
            const HEAP_INDEX right_child_is_largest =
                (HEAP_COMP(h->array[left_child], h->array[right_child])) > 0;

            if (right_child_is_largest)
                largest_child += right_child_is_largest;
        }

        // Swap if necessary
        if (HEAP_COMP(h->array[largest_child], h->array[parent]) < 0)
            HEAP_SWAP(h->array[largest_child], h->array[parent])
        else
            break;

        // Iterate down to the largest child
        parent = largest_child;
    }

    // Return the node that was removed from the heap
    return 0;
}

static inline HEAP_INDEX __EXPAND_CONCAT(HEAP_NAME, _size)(HEAP_NAME *h) { return h->size; }

static inline HEAP_TYPE __EXPAND_CONCAT(HEAP_NAME, _peek)(HEAP_NAME *h) { return h->array[0]; }

static inline int __EXPAND_CONCAT(HEAP_NAME, _build)(HEAP_TYPE *heap_array,
                                                     const HEAP_INDEX heap_array_size) {
    const HEAP_INDEX first_parent = heap_array_size / 2 - 1;

    // Use a FILO processing queue
    struct STACK_NAME nodes;
    if (__EXPAND_CONCAT(HEAP_NAME, _stack_create)(&nodes, heap_array_size / 2 + 1))
        return -1;

    // Queue all parent nodes for processing in the correct order
    for (HEAP_INDEX i = 0; i <= first_parent; i++)
#ifndef CTOOLS_ENABLE_DEBUG_ASSERT
        __EXPAND_CONCAT(STACK_NAME, _push)(&nodes, i);
#else
        assert(__EXPAND_CONCAT(STACK_NAME, _push)(&nodes, i) == 0);
#endif

    // While there are nodes left to process
    while (__EXPAND_CONCAT(STACK_NAME, _size)(&nodes)) {
        HEAP_INDEX parent;
        __EXPAND_CONCAT(STACK_NAME, _pop)(&nodes, &parent);

        // Find the index of both child nodes
        const HEAP_INDEX left_child = (parent + 1) * 2 - 1;
        const HEAP_INDEX right_child = (parent + 1) * 2;

        // Skip if the node has no children
        if (left_child >= heap_array_size)
            continue;

        // Find largest child
        HEAP_INDEX largest_child = left_child;
        if (right_child < heap_array_size)
            if (HEAP_COMP(heap_array[left_child], heap_array[right_child]) > 0)
                largest_child = right_child;

        // Swap if necessary
        if (HEAP_COMP(heap_array[largest_child], heap_array[parent]) < 0) {
            HEAP_SWAP(heap_array[largest_child], heap_array[parent]);

// Queue the node's children for processing
#ifndef CTOOLS_ENABLE_DEBUG_ASSERT
            __EXPAND_CONCAT(STACK_NAME, _push)(&nodes, left_child);
            __EXPAND_CONCAT(STACK_NAME, _push)(&nodes, right_child);
#else
            assert(__EXPAND_CONCAT(STACK_NAME, _push)(&nodes, left_child) == 0);
            assert(__EXPAND_CONCAT(STACK_NAME, _push)(&nodes, right_child) == 0);
#endif
        }
    }

    return 0;
}

static inline HEAP_INDEX __EXPAND_CONCAT(HEAP_NAME, _verify)(const HEAP_TYPE *heap_array,
                                                             const HEAP_INDEX heap_array_size) {
    HEAP_INDEX violations = 0;
    HEAP_INDEX parent = heap_array_size / 2 - 1;

    // Take care of the edge case where the last sub-tree only has one child.
    if ((heap_array_size & 1) == 0) {
        const HEAP_INDEX left_child = (parent + 1) * 2 - 1;
        violations += HEAP_COMP(heap_array[left_child], heap_array[parent]) < 0;
        parent--;
    }

    for (; parent < heap_array_size; parent--) {
        const HEAP_INDEX left_child = (parent + 1) * 2 - 1;
        const HEAP_INDEX right_child = (parent + 1) * 2;
        const HEAP_INDEX largest_child =
            HEAP_COMP(heap_array[left_child], heap_array[right_child]) < 0 ? left_child
                                                                           : right_child;
        violations += HEAP_COMP(heap_array[largest_child], heap_array[parent]) < 0;
    }

    return violations;
}

#undef HEAP_SWAP

#endif // HEAP_NO_IMPLEMENTATION
