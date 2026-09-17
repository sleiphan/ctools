#ifndef RADIX_TREE_DEFERRED_MEMCMP
#define RADIX_TREE_DEFERRED_MEMCMP

#include "ctools/trie/trie.h"
#include <stdbool.h>
#include <stdint.h>

#ifndef RTREE_SKIP_T
// The http standard recommends that servers support URIs with lengths of 8000 octets in protocol
// elements. Since a URI can consist mostly of a request path, this field must have space for at
// least 13 bits.
#define RTREE_SKIP_T uint16_t
#endif

#ifndef RTREE_INDEX_T
// Large API servers can have upwards of 300 endpoints, which results in 600 nodes in the worst case
// (binary tree structure). To index that amount of nodes, we need at least 10 bits. Using the full
// 16 bits allows this router to support 32768 endpoints in the worst case.
#define RTREE_INDEX_T uint16_t
#endif

struct rtree_node {
    // The next character in the string of the path
    // that this node represents.
    char character;

    // The node count of the tree that this node sits on top of. The count excludes
    // this node, meaning that a value of 0 identifies a leaf node.
    uint16_t tree_size;

    // Defines how many characters in the query string that the subnodes (of this node) skips.
    uint16_t key_length;

    // The value returned to the caller when searching through the trie.
    uint16_t value;

    uint8_t _padding[1];
};

struct rtree_setup_entry {
    const char *str;
    uint16_t value;
};

/**
 * Creates a radix tree from a string trie.
 *
 * @param top_node The top node of the trie that the radix tree should be converted from.
 */
struct rtree_node *rtree_create_from_trie(const struct trie_node *top_node);

struct rtree_node *rtree_create(const struct rtree_setup_entry *entries,
                                const uint16_t entry_count);

void rtree_destroy(struct rtree_node *top_node);

uint16_t rtree_search(const struct rtree_node *router_trie, const char *query_string,
                      const unsigned int query_string_length);

#endif // RADIX_TREE_DEFERRED_MEMCMP
