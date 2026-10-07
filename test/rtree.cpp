#include <gtest/gtest.h>

extern "C" {
#include "ctools/rtree.h"
}

const struct rtree_setup_entry entries_g[] = {
    (struct rtree_setup_entry){.str = "Hello World!", .value = 0},
    (struct rtree_setup_entry){.str = "Health", .value = 0},
    (struct rtree_setup_entry){.str = "Fire", .value = 0},
    (struct rtree_setup_entry){.str = "app", .value = 0},
    (struct rtree_setup_entry){.str = "apple", .value = 0},
    (struct rtree_setup_entry){.str = "application", .value = 0},
    (struct rtree_setup_entry){.str = "apply", .value = 0},
    (struct rtree_setup_entry){.str = "apt", .value = 0},
    (struct rtree_setup_entry){.str = "bat", .value = 0},
    (struct rtree_setup_entry){.str = "batch", .value = 0},
    (struct rtree_setup_entry){.str = "bath", .value = 0},
    (struct rtree_setup_entry){.str = "batman", .value = 0},
    (struct rtree_setup_entry){.str = "cat", .value = 0},
    (struct rtree_setup_entry){.str = "catalog", .value = 0},
    (struct rtree_setup_entry){.str = "cater", .value = 0},
    (struct rtree_setup_entry){.str = "catering", .value = 0},
    (struct rtree_setup_entry){.str = "dog", .value = 0},
    (struct rtree_setup_entry){.str = "dove", .value = 0},
    (struct rtree_setup_entry){.str = "dot", .value = 0},
    (struct rtree_setup_entry){.str = "door", .value = 0},
    (struct rtree_setup_entry){.str = "doom", .value = 0},
    (struct rtree_setup_entry){.str = "zoo", .value = 0},
    (struct rtree_setup_entry){.str = "zoom", .value = 0},
};

TEST(rtree, basic_usage) {
    struct rtree_setup_entry entries[sizeof(entries_g) / sizeof(struct rtree_setup_entry)];
    memcpy(entries, entries_g, sizeof(entries_g));

    // Assign different values to each entry
    for (int i = 0; i < sizeof(entries) / sizeof(struct rtree_setup_entry); i++)
        entries[i].value = i + 1;

    struct rtree radix_tree;
    if (rtree_create(&radix_tree, entries, sizeof(entries) / sizeof(struct rtree_setup_entry)))
        FAIL();

    // Check the value associated with each entry
    for (int i = 0; i < sizeof(entries) / sizeof(struct rtree_setup_entry); i++) {
        uint16_t value = rtree_search(&radix_tree, entries[i].str, strlen(entries[i].str));
        EXPECT_EQ(value, entries[i].value);
    }

    rtree_destroy(&radix_tree);
}

TEST(rtree, error_on_duplicate_keys) {
    struct rtree_setup_entry entries[] = {
        (struct rtree_setup_entry){.str = "helloworld", .value = 42},
        (struct rtree_setup_entry){.str = "helloworld", .value = 43},
    };

    // Assign different values to the value of each entry
    for (int i = 0; i < sizeof(entries) / sizeof(struct rtree_setup_entry); i++)
        entries[i].value = i + 1;

    struct rtree radix_tree;
    const int rv =
        rtree_create(&radix_tree, entries, sizeof(entries) / sizeof(struct rtree_setup_entry));
    EXPECT_NE(rv, 0);

    if (rv == 0)
        rtree_destroy(&radix_tree);
}
