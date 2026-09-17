#include <gtest/gtest.h>

#define KQUEUE_NAME my_kqueue
#define KQUEUE_TYPE int
#define KQUEUE_INDEX uint8_t
extern "C" {
#include "ctools/kqueue.h"
}
#undef KQUEUE_NAME
#undef KQUEUE_TYPE
#undef KQUEUE_INDEX

TEST(kqueue, queues_work_in_parallel) {
    int expected_1 = 1;
    int expected_2 = 2;
    int actual_1 = -1;
    int actual_2 = -1;

    struct my_kqueue q;
    EXPECT_EQ(my_kqueue_create(&q, 8, 2), 0);

    EXPECT_EQ(my_kqueue_push(&q, 1, expected_2), 0);
    EXPECT_EQ(my_kqueue_push(&q, 0, expected_1), 0);

    EXPECT_EQ(my_kqueue_pop(&q, 1, &actual_2), 0);
    EXPECT_EQ(my_kqueue_pop(&q, 0, &actual_1), 0);

    EXPECT_EQ(actual_1, expected_1);
    EXPECT_EQ(actual_2, expected_2);
}

TEST(kqueue, pop_as_first_action) {
    int expected = -1;
    int actual = expected;

    struct my_kqueue q;
    EXPECT_EQ(my_kqueue_create(&q, 8, 2), 0);

    EXPECT_NE(my_kqueue_pop(&q, 0, &actual), 0);
    EXPECT_EQ(expected, actual);
}

TEST(kqueue, FIFO_respected) {
    int expected[] = {1, 2, 3, 4};
    const int expected_count = sizeof(expected) / sizeof(expected[0]);

    int actual[expected_count];

    int *expected_p = expected;
    int *actual_p = actual;

    struct my_kqueue q;
    EXPECT_EQ(my_kqueue_create(&q, 8, 2), 0);

    // Push
    for (int i = 0; i < expected_count; i++)
        EXPECT_EQ(my_kqueue_push(&q, 0, *(expected_p++)), 0);

    // Pop
    for (int i = 0; i < expected_count; i++)
        EXPECT_EQ(my_kqueue_pop(&q, 0, actual_p++), 0);

    // Verify
    for (int i = 0; i < expected_count; i++)
        EXPECT_EQ(expected[i], actual[i]);
}

TEST(kqueue, fill_to_index_type_limits) {
    int expected[my_kqueue_max_size];
    int actual[my_kqueue_max_size];

    for (int i = 0; i < my_kqueue_max_size; i++) {
        expected[i] = i;
        actual[i] = -1;
    }

    struct my_kqueue q;
    EXPECT_EQ(my_kqueue_create(&q, my_kqueue_max_size, 2), 0);

    // Push
    for (int i = 0; i < my_kqueue_max_size; i++)
        EXPECT_EQ(my_kqueue_push(&q, 0, expected[i]), 0);

    // Check that new elements cannot be pushed
    EXPECT_NE(my_kqueue_push(&q, 0, (int)my_kqueue_max_size + 1), 0);

    // Pop
    for (int i = 0; i < my_kqueue_max_size; i++)
        EXPECT_EQ(my_kqueue_pop(&q, 0, &actual[i]), 0);

    // Check that new elements cannot be popped
    EXPECT_NE(my_kqueue_pop(&q, 0, actual), 0);

    // Verify
    for (int i = 0; i < my_kqueue_max_size; i++)
        EXPECT_EQ(expected[i], actual[i]);
}

TEST(kqueue, fill_to_index_type_limits_over_multiple_queues) {
    const uint8_t num_queues = 8;
    int expected[my_kqueue_max_size];
    int actual[my_kqueue_max_size];

    for (int i = 0; i < my_kqueue_max_size; i++) {
        expected[i] = i;
        actual[i] = -1;
    }

    struct my_kqueue q;
    EXPECT_EQ(my_kqueue_create(&q, my_kqueue_max_size, num_queues), 0);

    // Push
    for (int i = 0; i < my_kqueue_max_size; i++)
        EXPECT_EQ(my_kqueue_push(&q, i % num_queues, expected[i]), 0);

    // Check that new elements cannot be pushed
    EXPECT_NE(my_kqueue_push(&q, 0, (int)my_kqueue_max_size + 1), 0);

    // Pop
    for (int i = 0; i < my_kqueue_max_size; i++)
        EXPECT_EQ(my_kqueue_pop(&q, i % num_queues, &actual[i]), 0);

    // Check that new elements cannot be popped
    EXPECT_NE(my_kqueue_pop(&q, 0, actual), 0);

    // Verify
    for (int i = 0; i < my_kqueue_max_size; i++)
        EXPECT_EQ(expected[i], actual[i]);
}

TEST(kqueue, pop_from_empty_subqueue_whith_elements_in_other_subqueue) {
    struct my_kqueue q;
    EXPECT_EQ(my_kqueue_create(&q, 8, 4), 0);
    EXPECT_EQ(my_kqueue_push(&q, 0, 42), 0);

    int output = -1;
    EXPECT_NE(my_kqueue_pop(&q, 1, &output), 0);
}
