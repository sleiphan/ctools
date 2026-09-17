#ifndef CTOOLS_TRIE
#define CTOOLS_TRIE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct trie_node {
    char key;

    uint8_t subnodes_count;
    struct trie_node *subnodes;

    void *value;
};

struct trie_node *trie_create();

void trie_destroy(struct trie_node *node);

int trie_add(struct trie_node *top_node, const char *string, void *value);

void *trie_search(struct trie_node *top_node, const char *query_string);

#endif // CTOOLS_TRIE
