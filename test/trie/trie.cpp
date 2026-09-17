#include <gtest/gtest.h>

extern "C" {
#include "ctools/trie/trie.h"
}

TEST(trie, basic_usage) {
    struct trie_node *trie = trie_create();

    struct str_dict {
        const char *str;
        uint16_t value;
    };

    struct str_dict entries[] = {
        (struct str_dict){.str = "Hello World!", .value = 1},
        (struct str_dict){.str = "Health", .value = 2},
        (struct str_dict){.str = "Fire", .value = 3},
        (struct str_dict){.str = "app", .value = 4},
        (struct str_dict){.str = "apple", .value = 5},
        (struct str_dict){.str = "application", .value = 6},
        (struct str_dict){.str = "apply", .value = 7},
        (struct str_dict){.str = "apt", .value = 8},
        (struct str_dict){.str = "bat", .value = 9},
        (struct str_dict){.str = "batch", .value = 10},
        (struct str_dict){.str = "bath", .value = 11},
        (struct str_dict){.str = "batman", .value = 12},
        (struct str_dict){.str = "cat", .value = 13},
        (struct str_dict){.str = "catalog", .value = 14},
        (struct str_dict){.str = "cater", .value = 15},
        (struct str_dict){.str = "catering", .value = 16},
        (struct str_dict){.str = "dog", .value = 17},
        (struct str_dict){.str = "dove", .value = 18},
        (struct str_dict){.str = "dot", .value = 19},
        (struct str_dict){.str = "door", .value = 20},
        (struct str_dict){.str = "doom", .value = 21},
        (struct str_dict){.str = "zoo", .value = 22},
        (struct str_dict){.str = "zoom", .value = 23},
    };

    // Set the value of each entry
    for (int i = 0; i < sizeof(entries) / sizeof(struct str_dict); i++)
        entries[i].value = i + 1;

    // Add nodes to the trie
    for (int i = 0; i < sizeof(entries) / sizeof(struct str_dict); i++)
        trie_add(trie, entries[i].str, &entries[i].value);

    // Check the value associated with each entry
    for (int i = 0; i < sizeof(entries) / sizeof(struct str_dict); i++) {
        uint16_t value = *(uint16_t *)trie_search(trie, entries[i].str);
        EXPECT_EQ(value, entries[i].value);
    }

    trie_destroy(trie);
}
