#include <gtest/gtest.h>

extern "C" {
#include "ctools/bitset.h"
}

TEST(bitset, all_bits_work) {
    const unsigned int num_bits_to_test = 1 << 8;

    struct bitset bs;
    bitset_create(&bs, num_bits_to_test);

    for (unsigned int i = 0; i < num_bits_to_test; i++) {
        EXPECT_EQ(bitset_assign(&bs, i, 1), 0);
        EXPECT_NE(bitset_get(&bs, i), 0);
        EXPECT_EQ(bitset_assign(&bs, i, 0), 0);
        EXPECT_EQ(bitset_get(&bs, i), 0);
    }

    bitset_destroy(&bs);
}

TEST(bitset, search_includes_the_lower_bound) {
    struct bitset bs;
    bitset_create(&bs, 64);

    const unsigned int lower_bound = 5;
    const unsigned int upper_bound = 8;

    unsigned int result;
    bitset_assign(&bs, lower_bound, 1);
    bitset_assign(&bs, lower_bound + 1, 1);
    EXPECT_EQ(bitset_search_up(&bs, &result, lower_bound, upper_bound), 0);
    EXPECT_EQ(result, lower_bound);
    bitset_destroy(&bs);
}

TEST(bitset, search_excludes_the_upper_bound) {
    struct bitset bs;
    bitset_create(&bs, 64);

    const unsigned int lower_bound = 5;
    const unsigned int upper_bound = 8;

    unsigned int result;
    EXPECT_EQ(bitset_assign(&bs, upper_bound, 1), 0);
    EXPECT_EQ(bitset_search_up(&bs, &result, lower_bound, upper_bound), 1);
    bitset_destroy(&bs);
}

TEST(bitset, search_excludes_beyond_the_lower_bound) {
    struct bitset bs;
    bitset_create(&bs, 32);

    const unsigned int index_beyond_lower_bound = 2;
    const unsigned int lower_bound = 4;
    const unsigned int index_within_bounds = 6;
    const unsigned int upper_bound = 8;

    unsigned int result;
    EXPECT_EQ(bitset_assign(&bs, index_beyond_lower_bound, 1), 0);
    EXPECT_EQ(bitset_assign(&bs, index_within_bounds, 1), 0);

    EXPECT_EQ(bitset_search_up(&bs, &result, lower_bound, upper_bound), 0);
    EXPECT_EQ(result, index_within_bounds);

    bitset_destroy(&bs);
}

TEST(bitset, search_excludes_beyond_the_upper_bound) {
    struct bitset bs;
    bitset_create(&bs, 32);

    const unsigned int index_beyond_lower_bound = 2;
    const unsigned int lower_bound = 4;
    const unsigned int index_within_bounds = 6;
    const unsigned int upper_bound = 8;

    unsigned int result;
    EXPECT_EQ(bitset_assign(&bs, index_beyond_lower_bound, 1), 0);
    EXPECT_EQ(bitset_assign(&bs, index_within_bounds, 1), 0);

    EXPECT_EQ(bitset_search_up(&bs, &result, lower_bound, upper_bound), 0);
    EXPECT_EQ(result, index_within_bounds);

    bitset_destroy(&bs);
}

TEST(bitset, search_excludes_just_beyond_the_lower_bound) {
    struct bitset bs;
    bitset_create(&bs, 64);

    const unsigned int lower_bound = 5;
    const unsigned int upper_bound = 8;
    const unsigned int target_index = lower_bound - 1;

    unsigned int result;
    EXPECT_EQ(bitset_assign(&bs, target_index, 1), 0);
    EXPECT_EQ(bitset_search_up(&bs, &result, lower_bound, upper_bound), 1);
    bitset_destroy(&bs);
}

TEST(bitset, search_includes_just_within_the_upper_bound) {
    struct bitset bs;
    bitset_create(&bs, 64);

    const unsigned int lower_bound = 5;
    const unsigned int upper_bound = 8;
    const unsigned int target_index = upper_bound - 1;

    unsigned int result;
    EXPECT_EQ(bitset_assign(&bs, target_index, 1), 0);
    EXPECT_EQ(bitset_search_up(&bs, &result, lower_bound, upper_bound), 0);
    EXPECT_EQ(result, target_index);
    bitset_destroy(&bs);
}

TEST(bitset, search_traversing_entries) {
    struct bitset bs;
    bitset_create(&bs, 64 * 2);

    const unsigned int min = 60;
    const unsigned int max = 70;

    unsigned int result;
    for (unsigned int i = min; i < max; i++) {
        EXPECT_EQ(bitset_assign(&bs, i, 1), 0);
        EXPECT_EQ(bitset_search_up(&bs, &result, min, max), 0);
        EXPECT_EQ(bitset_assign(&bs, i, 0), 0);
        EXPECT_EQ(result, i);
    }

    bitset_destroy(&bs);
}

TEST(bitset, search_includes_first_bit) {
    struct bitset bs;
    bitset_create(&bs, 64);

    const unsigned int lower_bound = 0;
    const unsigned int upper_bound = 8;
    const unsigned int target_index = lower_bound;

    unsigned int result;
    EXPECT_EQ(bitset_assign(&bs, target_index, 1), 0);
    EXPECT_EQ(bitset_assign(&bs, target_index + 1, 1), 0);
    EXPECT_EQ(bitset_assign(&bs, target_index + 2, 1), 0);
    EXPECT_EQ(bitset_search_up(&bs, &result, lower_bound, upper_bound), 0);
    EXPECT_EQ(result, target_index);

    bitset_destroy(&bs);
}

TEST(bitset, search_includes_final_bit_in_entry) {
    struct bitset bs;
    bitset_create(&bs, 64 * 2);

    const unsigned int lower_bound = 60;
    const unsigned int upper_bound = 65;
    const unsigned int target_index = 64;

    unsigned int result;
    EXPECT_EQ(bitset_assign(&bs, target_index, 1), 0);
    EXPECT_EQ(bitset_search_up(&bs, &result, lower_bound, upper_bound), 0);
    EXPECT_EQ(result, target_index);

    bitset_destroy(&bs);
}
