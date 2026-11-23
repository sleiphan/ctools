#include <gtest/gtest.h>

extern "C" {
#include "ctools/rtree.h"
}

#undef RTREE_NAME
#undef CTOOLS_RTREE_INDEX
#define RTREE_NAME rtree8
#define CTOOLS_RTREE_INDEX uint8_t
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

    const uint16_t rtree_size = (sizeof(entries) / sizeof(struct rtree_setup_entry)) * 2 - 1;
    struct rtree_node radix_tree[rtree_size];

    if (rtree_create(radix_tree, rtree_size, entries,
                     sizeof(entries) / sizeof(struct rtree_setup_entry), NULL))
        FAIL();

    // Check the value associated with each entry
    for (int i = 0; i < sizeof(entries) / sizeof(struct rtree_setup_entry); i++) {
        uint16_t value = 0;
        rtree_search(radix_tree, entries[i].str, strlen(entries[i].str), &value);
        EXPECT_EQ(value, entries[i].value);
    }
}

TEST(rtree_create, error_on_duplicate_keys) {
    struct rtree_setup_entry entries[] = {
        (struct rtree_setup_entry){.str = "helloworld", .value = 42},
        (struct rtree_setup_entry){.str = "helloworld", .value = 43},
    };

    const uint16_t rtree_size = (sizeof(entries) / sizeof(struct rtree_setup_entry)) * 2 - 1;
    struct rtree_node radix_tree[rtree_size];

    const int rv = rtree_create(radix_tree, rtree_size, entries,
                                sizeof(entries) / sizeof(struct rtree_setup_entry), NULL);
    EXPECT_NE(rv, 0);
}

TEST(rtree_create, error_on_empty_entry_list) {
    struct rtree_setup_entry entries[] = {
        (struct rtree_setup_entry){.str = "sole_entry", .value = 42},
    };

    const uint16_t rtree_size = (sizeof(entries) / sizeof(struct rtree_setup_entry)) * 2 - 1;
    struct rtree_node radix_tree[rtree_size];

    const int rv = rtree_create(radix_tree, rtree_size, entries, 0, NULL);
    EXPECT_EQ(errno, EINVAL);
    EXPECT_NE(rv, 0);
}

TEST(rtree_create, error_on_NULL_rtree) {
    struct rtree_setup_entry entries[] = {
        (struct rtree_setup_entry){.str = "sole_entry", .value = 42},
    };

    const uint16_t rtree_size = (sizeof(entries) / sizeof(struct rtree_setup_entry)) * 2 - 1;
    struct rtree_node radix_tree[rtree_size];

    const int rv = rtree_create(NULL, rtree_size, entries, 0, NULL);
    EXPECT_EQ(errno, EINVAL);
    EXPECT_NE(rv, 0);
}

TEST(rtree_create, error_on_NULL_entry_list) {
    const uint16_t rtree_size = 1 * 2 - 1;
    struct rtree_node radix_tree[rtree_size];

    const int rv = rtree_create(radix_tree, rtree_size, NULL, 42, NULL);
    EXPECT_EQ(errno, EINVAL);
    EXPECT_NE(rv, 0);
}

TEST(rtree_search, error_on_NULL_rtree_object) {
    const char *key_query = "anything";
    uint16_t value = 0;

    const int rv = rtree_search(NULL, key_query, strlen(key_query), &value);

    EXPECT_EQ(errno, EINVAL);
    EXPECT_LT(rv, 0);
}

TEST(rtree_search, error_on_NULL_query_string) {
    struct rtree_setup_entry entries[sizeof(entries_g) / sizeof(struct rtree_setup_entry)];
    memcpy(entries, entries_g, sizeof(entries_g));

    // Assign different values to each entry
    for (int i = 0; i < sizeof(entries) / sizeof(struct rtree_setup_entry); i++)
        entries[i].value = i + 1;

    const uint16_t rtree_size = (sizeof(entries) / sizeof(struct rtree_setup_entry)) * 2 - 1;
    struct rtree_node radix_tree[rtree_size];

    if (rtree_create(radix_tree, rtree_size, entries,
                     sizeof(entries) / sizeof(struct rtree_setup_entry), NULL))
        FAIL();

    uint16_t value = 0;
    const int rv = rtree_search(radix_tree, NULL, strlen(entries[0].str), &value);
    EXPECT_EQ(errno, EINVAL);
    EXPECT_LT(rv, 0);
}

TEST(rtree_search, error_on_NULL_value_out_pointer) {
    struct rtree_setup_entry entries[sizeof(entries_g) / sizeof(struct rtree_setup_entry)];
    memcpy(entries, entries_g, sizeof(entries_g));

    const uint16_t rtree_size = (sizeof(entries) / sizeof(struct rtree_setup_entry)) * 2 - 1;
    struct rtree_node radix_tree[rtree_size];

    // Assign different values to each entry
    for (int i = 0; i < sizeof(entries) / sizeof(struct rtree_setup_entry); i++)
        entries[i].value = i + 1;

    if (rtree_create(radix_tree, rtree_size, entries,
                     sizeof(entries) / sizeof(struct rtree_setup_entry), NULL))
        FAIL();

    uint16_t value = 0;
    const int rv = rtree_search(radix_tree, entries[0].str, strlen(entries[0].str), NULL);
    EXPECT_EQ(errno, EINVAL);
    EXPECT_LT(rv, 0);
}

TEST(rtree_create, enforces_entry_count_limit_to_prevent_node_count_overflow) {
    const uint16_t entries_count = 260;
    const int key_length = 8;

    struct rtree8_setup_entry entries[entries_count];
    char keys[entries_count][key_length];

    const uint8_t rtree_size = ((uint8_t)-1);
    struct rtree8_node radix_tree[rtree_size];

    memset(keys, 0, sizeof(keys));
    for (uint16_t i = 0; i < entries_count; i++) {
        EXPECT_GT(snprintf(keys[i], key_length, "%d", i + 100), 0);
        entries[i] = (struct rtree8_setup_entry){
            .str = keys[i],
            .value = i,
        };
    }

    int rv = -1;
    int err = -1;

    rv = rtree8_create(radix_tree, rtree_size, entries, 127, NULL);
    EXPECT_EQ(rv, 0);

    rv = rtree8_create(radix_tree, rtree_size, entries, 128, NULL);
    EXPECT_EQ(rv, 0);

    errno = 0; // Reset errno
    rv = rtree8_create(radix_tree, rtree_size, entries, 129, NULL);
    err = errno;
    EXPECT_EQ(rv, -1);
    EXPECT_NE(err, 0);
}

/**
 * Guards against scanning past the current subtree into an unrelated branch.
 * Using the whole-tree bound let "ac" escape the "a" subtree and return "ca"'s
 * value instead of reporting no match.
 */
TEST(rtree_search, does_not_match_outside_current_subtree) {
    struct rtree_setup_entry entries[] = {(struct rtree_setup_entry){.str = "aa", .value = 1},
                                          (struct rtree_setup_entry){.str = "ab", .value = 2},
                                          (struct rtree_setup_entry){.str = "ca", .value = 3},
                                          (struct rtree_setup_entry){.str = "cb", .value = 4}};

    const uint16_t rtree_size = (sizeof(entries) / sizeof(struct rtree_setup_entry)) * 2 - 1;
    struct rtree_node radix_tree[rtree_size];

    // Assign different values to each entry
    for (int i = 0; i < sizeof(entries) / sizeof(struct rtree_setup_entry); i++)
        entries[i].value = i + 1;

    if (rtree_create(radix_tree, rtree_size, entries,
                     sizeof(entries) / sizeof(struct rtree_setup_entry), NULL))
        FAIL();

    RTREE_VALUE value = 0;
    const int rv = rtree_search(radix_tree, "ac", 2, &value);
    EXPECT_EQ(rv, 0);
    EXPECT_EQ(value, 0);
}
