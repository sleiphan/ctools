#ifndef RTREE_NO_INTERFACE

struct rtree_node;

struct rtree {
    struct rtree_node *nodes;
};

struct rtree_setup_entry {
    const char *str;
    uint16_t value;
};

struct rtree;

int rtree_create(struct rtree *rtree, const struct rtree_setup_entry *entries,
                 const uint16_t entry_count);

void rtree_destroy(struct rtree *rtree);

uint16_t rtree_search(const struct rtree *rtree, const char *query_string,
                      const unsigned int query_string_length);

#endif // RTREE_NO_INTERFACE

#ifndef RTREE_NO_IMPLEMENTATION

#ifndef CTOOLS_RTREE_SKIP_T
// The http standard recommends that servers support URIs with lengths of 8000 octets in protocol
// elements. Since a URI can consist mostly of a request path, this field must have space for at
// least 13 bits.
#define CTOOLS_RTREE_SKIP_T uint16_t
#endif

#ifndef CTOOLS_RTREE_INDEX_T
// Large API servers can have upwards of 300 endpoints, which results in 600 nodes in the worst case
// (binary tree structure). To index that amount of nodes, we need at least 10 bits. Using the full
// 16 bits allows this router to support 32768 endpoints in the worst case.
#define CTOOLS_RTREE_INDEX_T uint16_t
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "min.h"

struct rtree_node {
    // The next character in the string of the path
    // that this node represents.
    char character;

    // The node count of the tree that this node sits on top of. The count excludes
    // this node, meaning that a value of 0 identifies a leaf node.
    uint16_t tree_size;

    // Defines how many characters in the query string that the subnodes (of this node) skips.
    uint16_t key_length;

    // The value returned to the caller when searching through the rtree.
    uint16_t value;

    uint8_t _padding[1];
};

int _rtree_sort_entries_by_key_cmp(const void *a, const void *b) {
    struct rtree_setup_entry *first = (struct rtree_setup_entry *)a;
    struct rtree_setup_entry *second = (struct rtree_setup_entry *)b;

    return strcmp(first->str, (const char *)second->str);
}

unsigned int _rtree_build_internal(const struct rtree_setup_entry *entries,
                                   const uint16_t entry_count, struct rtree_node **nodes,
                                   uint16_t *nodes_size, uint16_t *nodes_capactity,
                                   const uint16_t key_idx) {
    // 1. Seek to first column with different characters
    // 2. Create a new node
    // 3. Recursively call this function for each diverging group

    uint16_t current_key_char_idx = key_idx;

    bool duplicate_keys = false;

    // Seek to first column with different characters
    for (bool divergence_found = false; !divergence_found;) {

        char current_char = entries[0].str[current_key_char_idx];
        bool null_encountered = false;

        for (uint16_t entry_idx = 0; entry_idx < entry_count; entry_idx++) {
            bool unequal_character = entries[entry_idx].str[current_key_char_idx] != current_char;
            bool is_null = entries[entry_idx].str[current_key_char_idx] == '\0';
            if (unequal_character || is_null)
                divergence_found = true;

            // Check for duplicates
            duplicate_keys |= null_encountered & is_null;
            null_encountered |= is_null;
        }

        current_key_char_idx += !divergence_found;
    }

    // Error out when encountering duplicate keys
    if (duplicate_keys) {
        errno = EINVAL;
        return 0;
    }

    // Increase capacity if we are about to blow past it
    if (*nodes_size == *nodes_capactity) {
        struct rtree_node *new_nodes =
            (struct rtree_node *)realloc(*nodes, *nodes_capactity * 2 * sizeof(struct rtree_node));
        if (!new_nodes)
            return 0;
        *nodes_capactity *= 2;
        *nodes = new_nodes;
    }

    const uint16_t key_length = current_key_char_idx - key_idx;

    // Create a new node
    uint32_t active_node_idx = (*nodes_size)++;
    (*nodes)[active_node_idx] = (struct rtree_node){
        .character = entries->str[key_idx],
        .tree_size = 1, // Assume this is a leaf node
        .key_length = key_length,
        .value = entries->value,
        ._padding = 0,
    };

    // End if this is a leaf node
    if (entry_count == 1)
        return entry_count;
    // Else: this is a parent node

    // Scan the character at the same index in every key
    for (uint16_t range_begin = 0; range_begin < entry_count;) {
        const char current_char = entries[range_begin].str[current_key_char_idx];

        // Seek to next divergence
        uint16_t range_end = range_begin + 1;
        while (range_end < entry_count &&
               current_char == entries[range_end].str[current_key_char_idx])
            range_end++;

        // Recursively call this procedure with the set of equal characters in the index
        const unsigned int num_new_nodes =
            _rtree_build_internal(&entries[range_begin], range_end - range_begin, nodes, nodes_size,
                                  nodes_capactity, current_key_char_idx);

        // If the operation failed, exit
        if (!num_new_nodes)
            return num_new_nodes;

        (*nodes)[active_node_idx].tree_size += num_new_nodes;

        range_begin = range_end;
    }

    return (*nodes)[active_node_idx].tree_size;
}

int rtree_create(struct rtree *rtree, const struct rtree_setup_entry *entries,
                 const uint16_t entry_count) {

    if (entry_count == 0 || entries == NULL) {
        errno = EINVAL;
        return -1;
    }

    static const unsigned int INITIAL_NODE_CAPACITY = 8;

    struct rtree_setup_entry *sorted_entries =
        (struct rtree_setup_entry *)malloc(entry_count * sizeof(struct rtree_setup_entry));

    if (!sorted_entries)
        return -1;

    struct rtree_node *nodes =
        (struct rtree_node *)malloc(INITIAL_NODE_CAPACITY * sizeof(struct rtree_node));

    if (!nodes) {
        free(sorted_entries);
        return -1;
    }

    memcpy(sorted_entries, entries, entry_count * sizeof(struct rtree_setup_entry));
    qsort(sorted_entries, entry_count, sizeof(struct rtree_setup_entry),
          _rtree_sort_entries_by_key_cmp);

    uint16_t node_count = 0;
    uint16_t node_capacity = INITIAL_NODE_CAPACITY;
    unsigned int num_nodes =
        _rtree_build_internal(sorted_entries, entry_count, &nodes, &node_count, &node_capacity, 0);

    free(sorted_entries);

    if (!num_nodes) {
        free(nodes);
        return -1;
    }

    *rtree = (struct rtree){
        .nodes = nodes,
    };

    return 0;
}

void rtree_destroy(struct rtree *rtree) { free(rtree->nodes); }

uint16_t rtree_search(const struct rtree *rtree, const char *query_string,
                      const unsigned int query_string_length) {
    // An iterator to seek through the query_string
    unsigned int query_string_it = 0;

    // We start searching from the top node
    uint16_t current_node = 0;

    // Loop until we hit a leaf node.
    while (rtree->nodes[current_node].tree_size) {

        // Seek to to the character that this node's subnodes compares with.
        query_string_it += rtree->nodes[current_node].key_length;

        // If we have scanned the exact length of the query string, we have to return whatever value
        // registered in the current node. We might have reached a branching node that contains a
        // value. This is a typical case when e.g. an API has endpoints like these:
        //   - /api/user
        //   - /api/user/login
        //   - /api/user/create
        if (query_string_it == query_string_length)
            break;

        // If the subnodes wants to match with a character that is beyond the length of
        // the query_string, the query string does not match anything in this trie.
        if (query_string_it >= query_string_length)
            return -1;

        // If a matching subnode is found, it will be stored here.
        uint16_t matching_node = 0;

        // Search for a subnode that matches with the query string
        for (uint16_t subnode = current_node + 1;
             !matching_node & (subnode < rtree->nodes->tree_size);
             subnode += rtree->nodes[subnode].tree_size)
            if (rtree->nodes[subnode].character == query_string[query_string_it])
                matching_node = subnode;

        // If none of the subnodes match, the query string does not exist in this trie.
        if (!matching_node)
            return -1;

        // Continue processing from the subnode.
        current_node = matching_node;
    }

    return rtree->nodes[current_node].value;
}

#endif // RTREE_NO_IMPLEMENTATION
