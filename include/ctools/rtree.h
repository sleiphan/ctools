#ifndef RTREE_NAME
#define RTREE_NAME rtree
#endif

#ifndef RTREE_VALUE
#include <stdint.h>
#define RTREE_VALUE uint16_t
#endif

#ifndef CTOOLS_RTREE_SKIP
#include <stdint.h>
// The http standard recommends that servers support URIs with lengths of 8000 octets in protocol
// elements. Since a URI can consist mostly of a request path, this field must have space for at
// least 13 bits.
#define CTOOLS_RTREE_SKIP uint16_t
#endif

#ifndef CTOOLS_RTREE_INDEX
#include <stdint.h>
// Large API servers can have upwards of 300 endpoints, which results in 600 nodes in the worst case
// (binary tree structure). To index that amount of nodes, we need at least 10 bits. Using the full
// 16 bits allows this router to support 32768 endpoints in the worst case.
#define CTOOLS_RTREE_INDEX uint16_t
#endif

#include "define_concat.h"

#ifndef RTREE_NO_INTERFACE

struct __EXPAND_CONCAT(RTREE_NAME, _node);

struct RTREE_NAME {
    struct __EXPAND_CONCAT(RTREE_NAME, _node) * nodes;
};

struct __EXPAND_CONCAT(RTREE_NAME, _setup_entry) {
    const char *str;
    RTREE_VALUE value;
};

struct RTREE_NAME;

int __EXPAND_CONCAT(RTREE_NAME,
                    _create)(struct RTREE_NAME *rtree,
                             const struct __EXPAND_CONCAT(RTREE_NAME, _setup_entry) * entries,
                             const CTOOLS_RTREE_INDEX entry_count);

void __EXPAND_CONCAT(RTREE_NAME, _destroy)(struct RTREE_NAME *rtree);

RTREE_VALUE __EXPAND_CONCAT(RTREE_NAME, _search)(const struct RTREE_NAME *rtree,
                                                 const char *query_string,
                                                 const unsigned int query_string_length);

#endif // RTREE_NO_INTERFACE

#ifndef RTREE_NO_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

#include "min.h"

struct __EXPAND_CONCAT(RTREE_NAME, _node) {
    // The next character in the string of the path
    // that this node represents.
    char character;

    // The node count of the tree that this node sits on top of. The count excludes
    // this node, meaning that a value of 0 identifies a leaf node.
    CTOOLS_RTREE_INDEX tree_size;

    // Defines how many characters in the query string that the subnodes (of this node) skips.
    CTOOLS_RTREE_SKIP key_length;

    // The value returned to the caller when searching through the rtree.
    RTREE_VALUE value;
};

int __EXPAND_CONCAT(__EXPAND_CONCAT(_, RTREE_NAME), _sort_entries_by_key_cmp)(const void *a,
                                                                              const void *b) {
    struct __EXPAND_CONCAT(RTREE_NAME, _setup_entry) *first =
        (struct __EXPAND_CONCAT(RTREE_NAME, _setup_entry) *)a;
    struct __EXPAND_CONCAT(RTREE_NAME, _setup_entry) *second =
        (struct __EXPAND_CONCAT(RTREE_NAME, _setup_entry) *)b;

    return strcmp(first->str, (const char *)second->str);
}

unsigned int __EXPAND_CONCAT(__EXPAND_CONCAT(_, RTREE_NAME), _build_internal)(
    const struct __EXPAND_CONCAT(RTREE_NAME, _setup_entry) * entries,
    const CTOOLS_RTREE_INDEX entry_count, struct __EXPAND_CONCAT(RTREE_NAME, _node) * *nodes,
    CTOOLS_RTREE_INDEX *nodes_size, CTOOLS_RTREE_INDEX *nodes_capactity,
    const CTOOLS_RTREE_INDEX key_idx) {

    // 1. Seek to first column with different characters
    // 2. Create a new node
    // 3. Recursively call this function for each diverging group

    CTOOLS_RTREE_INDEX current_key_char_idx = key_idx;

    bool duplicate_keys = false;

    // Seek to first column with different characters
    for (bool divergence_found = false; !divergence_found;) {

        char current_char = entries[0].str[current_key_char_idx];
        bool null_encountered = false;

        for (CTOOLS_RTREE_INDEX entry_idx = 0; entry_idx < entry_count; entry_idx++) {
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
        struct __EXPAND_CONCAT(RTREE_NAME, _node) *new_nodes =
            (struct __EXPAND_CONCAT(RTREE_NAME, _node) *)realloc(
                *nodes, *nodes_capactity * 2 * sizeof(struct __EXPAND_CONCAT(RTREE_NAME, _node)));
        if (!new_nodes)
            return 0;
        *nodes_capactity *= 2;
        *nodes = new_nodes;
    }

    const CTOOLS_RTREE_SKIP key_length = current_key_char_idx - key_idx;

    // Create a new node
    const CTOOLS_RTREE_INDEX active_node_idx = (*nodes_size)++;
    (*nodes)[active_node_idx] = (struct __EXPAND_CONCAT(RTREE_NAME, _node)){
        .character = entries->str[key_idx],
        .tree_size = 1, // Assume this is a leaf node
        .key_length = key_length,
        .value = entries->value,
    };

    // End if this is a leaf node
    if (entry_count == 1)
        return entry_count;
    // Else: this is a parent node

    // Scan the character at the same index in every key
    for (CTOOLS_RTREE_INDEX range_begin = 0; range_begin < entry_count;) {
        const char current_char = entries[range_begin].str[current_key_char_idx];

        // Seek to next divergence
        CTOOLS_RTREE_INDEX range_end = range_begin + 1;
        while (range_end < entry_count &&
               current_char == entries[range_end].str[current_key_char_idx])
            range_end++;

        // Recursively call this procedure with the set of equal characters in the index
        const unsigned int num_new_nodes =
            __EXPAND_CONCAT(__EXPAND_CONCAT(_, RTREE_NAME),
                            _build_internal)(&entries[range_begin], range_end - range_begin, nodes,
                                             nodes_size, nodes_capactity, current_key_char_idx);

        // If the operation failed, exit
        if (!num_new_nodes)
            return num_new_nodes;

        (*nodes)[active_node_idx].tree_size += num_new_nodes;

        range_begin = range_end;
    }

    return (*nodes)[active_node_idx].tree_size;
}

int __EXPAND_CONCAT(RTREE_NAME,
                    _create)(struct RTREE_NAME *rtree,
                             const struct __EXPAND_CONCAT(RTREE_NAME, _setup_entry) * entries,
                             const CTOOLS_RTREE_INDEX entry_count) {

    if (entry_count == 0 || entries == NULL) {
        errno = EINVAL;
        return -1;
    }

    static const unsigned int INITIAL_NODE_CAPACITY = 8;

    struct __EXPAND_CONCAT(RTREE_NAME, _setup_entry) *sorted_entries =
        (struct __EXPAND_CONCAT(RTREE_NAME, _setup_entry) *)malloc(
            entry_count * sizeof(struct __EXPAND_CONCAT(RTREE_NAME, _setup_entry)));

    if (!sorted_entries)
        return -1;

    struct __EXPAND_CONCAT(RTREE_NAME, _node) *nodes =
        (struct __EXPAND_CONCAT(RTREE_NAME, _node) *)malloc(
            INITIAL_NODE_CAPACITY * sizeof(struct __EXPAND_CONCAT(RTREE_NAME, _node)));

    if (!nodes) {
        free(sorted_entries);
        return -1;
    }

    memcpy(sorted_entries, entries,
           entry_count * sizeof(struct __EXPAND_CONCAT(RTREE_NAME, _setup_entry)));
    qsort(sorted_entries, entry_count, sizeof(struct __EXPAND_CONCAT(RTREE_NAME, _setup_entry)),
          __EXPAND_CONCAT(__EXPAND_CONCAT(_, RTREE_NAME), _sort_entries_by_key_cmp));

    CTOOLS_RTREE_INDEX node_count = 0;
    CTOOLS_RTREE_INDEX node_capacity = INITIAL_NODE_CAPACITY;
    unsigned int num_nodes = __EXPAND_CONCAT(__EXPAND_CONCAT(_, RTREE_NAME), _build_internal)(
        sorted_entries, entry_count, &nodes, &node_count, &node_capacity, 0);

    free(sorted_entries);

    if (!num_nodes) {
        free(nodes);
        return -1;
    }

    *rtree = (struct RTREE_NAME){
        .nodes = nodes,
    };

    return 0;
}

void __EXPAND_CONCAT(RTREE_NAME, _destroy)(struct RTREE_NAME *rtree) { free(rtree->nodes); }

RTREE_VALUE __EXPAND_CONCAT(RTREE_NAME, _search)(const struct RTREE_NAME *rtree,
                                                 const char *query_string,
                                                 const unsigned int query_string_length) {
    // An iterator to seek through the query_string
    unsigned int query_string_it = 0;

    // We start searching from the top node
    CTOOLS_RTREE_INDEX current_node = 0;

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
        CTOOLS_RTREE_INDEX matching_node = 0;

        // Search for a subnode that matches with the query string
        for (CTOOLS_RTREE_INDEX subnode = current_node + 1;
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
