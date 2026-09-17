#include "ctools/trie/trie.h"

struct print_node {
    struct trie_node *node_ptr;
    unsigned int indentation;
};

#define STACK_EXT_DYNAMIC_SIZE
#define STACK_NAME node_stack
#define STACK_INDEX unsigned int
#define STACK_TYPE struct trie_node *
#include "ctools/stack.h"
#undef STACK_NAME
#undef STACK_INDEX
#undef STACK_TYPE

/**
 * Resets the fields of the given trie node to its default values.
 *
 * @param node The node to reset.
 */
static inline void _trie_node_init(struct trie_node *node) {
    *node = (struct trie_node){.key = 0, .subnodes_count = 0, .subnodes = NULL, .value = NULL};
}

struct trie_node *trie_create() {
    struct trie_node *new_node = (struct trie_node *)malloc(sizeof(struct trie_node));

    if (!new_node) {
        perror("malloc");
        return NULL;
    }

    _trie_node_init(new_node);

    return new_node;
}

void trie_node_destroy(struct trie_node *node) {
    if (node->subnodes_count)
        free(node->subnodes);

    _trie_node_init(node);
}

void trie_destroy(struct trie_node *top_node) {
    struct trie_node *current_node;
    struct node_stack deletion_queue;
    node_stack_create(&deletion_queue, 0);
    node_stack_push(&deletion_queue, top_node);

    // Delete nodes from the top down
    while (!node_stack_pop(&deletion_queue, &current_node)) {
        // First, empty the subnode array into the deletion stack before freeing the array
        for (int i = 0; i < current_node->subnodes_count; i++)
            node_stack_push(&deletion_queue, &current_node->subnodes[i]);

        trie_node_destroy(current_node);
    }
}

struct trie_node *_trie_search(struct trie_node *top_node, const char *string,
                               unsigned int string_length, unsigned int *depth) {
    const char *string_it = string;
    struct trie_node *current_node = top_node;

    while (string_length--) {
        if (current_node->subnodes_count == 0)
            break;

        struct trie_node *next_node = 0;

        for (int i = 0; i < current_node->subnodes_count; i++) {
            if (current_node->subnodes[i].key == *string_it)
                next_node = &current_node->subnodes[i];
        }

        if (!next_node)
            break;

        current_node = next_node;
        string_it++;
        *depth += 1;
    }

    return current_node;
}

void *trie_search(struct trie_node *top_node, const char *query_string) {
    unsigned int search_depth = 0;
    const unsigned int string_length = strlen(query_string);
    struct trie_node *result_node =
        _trie_search(top_node, query_string, string_length, &search_depth);

    if ((search_depth < string_length) | (result_node == NULL))
        return NULL;

    return result_node->value;
}

int trie_add(struct trie_node *top_node, const char *string, void *value) {
    unsigned int search_depth = 0;
    const unsigned int string_length = strlen(string);
    struct trie_node *node_to_extend = _trie_search(top_node, string, string_length, &search_depth);

    for (; search_depth < string_length; search_depth++) {
        if (node_to_extend->subnodes_count) {
            const unsigned int new_array_size = node_to_extend->subnodes_count + 1;
            struct trie_node *new_array = (struct trie_node *)realloc(
                node_to_extend->subnodes, new_array_size * sizeof(struct trie_node));

            if (!new_array) {
                perror("realloc");
                return -1;
            }

            _trie_node_init(&new_array[node_to_extend->subnodes_count]);

            node_to_extend->subnodes = new_array;
            node_to_extend->subnodes_count = new_array_size;
        } else {
            node_to_extend->subnodes = (struct trie_node *)malloc(sizeof(struct trie_node));
            _trie_node_init(node_to_extend->subnodes);
            node_to_extend->subnodes_count = 1;
        }

        node_to_extend = &node_to_extend->subnodes[node_to_extend->subnodes_count - 1];
        _trie_node_init(node_to_extend);
        node_to_extend->key = string[search_depth];
    }

    node_to_extend->value = value;
    return 0;
}
