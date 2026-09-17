#ifndef CTOOLS_BITSET_INTERFACE
#define CTOOLS_BITSET_INTERFACE

#include <stdint.h>

typedef unsigned int bitset_index;
typedef uint64_t bitset_entry;

struct bitset {
    bitset_entry *entries;
    bitset_index bit_count;
};

int bitset_create(struct bitset *bs, const bitset_index bit_count);
static inline void bitset_destroy(struct bitset *bs);
static inline int bitset_assign(struct bitset *bs, const bitset_index index, int value);
static inline int bitset_get(struct bitset *bs, const bitset_index index);
int bitset_search_up(const struct bitset *bs, bitset_index *dst, const bitset_index from,
                     const bitset_index to);

#endif // CTOOLS_BITSET_INTERFACE

#ifndef BITSET_NO_IMPLEMENTATION
#include "ctools/bitset_impl.h"
#endif
