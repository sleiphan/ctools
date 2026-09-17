#include <gtest/gtest.h>

#define QUEUE_NAME second_queue_type
#define QUEUE_TYPE int
#define QUEUE_INDEX unsigned int
// This #include section is only to verify that the header
// supports multiple inclusions in a compilation unit
extern "C" {
#include "ctools/queue.h"
}

#undef QUEUE_NAME
#undef QUEUE_INDEX
#define QUEUE_NAME queue
#define QUEUE_INDEX unsigned char
extern "C" {
#include "ctools/queue.h"
#include <errno.h>
}

TEST(queue, mask_is_created_correctly) {
    queue q;
    queue_create(&q, 7);
    EXPECT_EQ(q.capacity_mask, 7);
    queue_destroy(&q);

    queue_create(&q, 8);
    EXPECT_EQ(q.capacity_mask, 15);
    queue_destroy(&q);

    queue_create(&q, 9);
    EXPECT_EQ(q.capacity_mask, 15);
    queue_destroy(&q);
}

TEST(queue, not_lying_about_minimum_capacity) {
    const int queue_min_size = 8;

    queue q;
    queue_create(&q, queue_min_size);

    int numbers_pushed = 0;

    while ((numbers_pushed < queue_min_size) & !queue_push(&q, 0))
        numbers_pushed++;

    EXPECT_EQ(numbers_pushed, queue_min_size);

    queue_destroy(&q);
}

TEST(queue, rejects_capacities_over_supported_values) {
    queue q;
    int err = queue_create(&q, queue_max_size);

    EXPECT_EQ(err, EINVAL);

    if (!err)
        queue_destroy(&q);
}

TEST(queue, base_case) {
    constexpr QUEUE_INDEX capacity = (1 << 3) - 1;
    constexpr QUEUE_INDEX half_capacity = capacity / 2;

    queue q;
    queue_create(&q, capacity);

    int error = 0;

    // Push the queue halfway through its capacity
    for (QUEUE_TYPE i = 0; i < half_capacity; i++)
        error |= queue_push(&q, i);

    // Pop the queue halfway through its capacity
    for (QUEUE_TYPE i = 0; i < half_capacity; i++) {
        QUEUE_TYPE sink;
        error |= queue_pop(&q, &sink);
        EXPECT_EQ(sink, i);
    }

    for (QUEUE_TYPE i = 0; i < capacity; i++)
        error |= queue_push(&q, i);

    for (QUEUE_TYPE i = 0; i < capacity; i++) {
        QUEUE_TYPE sink;
        error |= queue_pop(&q, &sink);
        EXPECT_EQ(sink, i);
    }

    // Verify that no errors occurred
    EXPECT_EQ(error, 0);
    EXPECT_EQ(errno, 0);

    queue_destroy(&q);
}

TEST(queue, emptying_and_reusing) {
    const int test_value_1 = 42;
    const int test_value_2 = 420;
    int pop_1;
    int pop_2;

    queue q;
    queue_create(&q, 20);

    for (int i = 0; i < 10; i++) {
        queue_push(&q, test_value_1);
        queue_push(&q, test_value_2);

        queue_pop(&q, &pop_1);
        queue_pop(&q, &pop_2);

        EXPECT_EQ(pop_1, test_value_1);
        EXPECT_EQ(pop_2, test_value_2);
    }

    queue_destroy(&q);
}

TEST(queue, size_reports_correctly) {
    constexpr QUEUE_INDEX capacity = queue_max_size - 1;
    constexpr QUEUE_INDEX half_capacity = capacity / 2;

    queue q;
    queue_create(&q, capacity);
    EXPECT_EQ(queue_size(&q), 0);

    // Push the queue halfway through its capacity
    for (QUEUE_TYPE i = 0; i < half_capacity; i++) {
        queue_push(&q, i);
        EXPECT_EQ(queue_size(&q), i + 1);
    }

    // Pop the queue halfway through its capacity
    for (QUEUE_TYPE i = half_capacity; i > 0; i--) {
        QUEUE_TYPE sink;
        queue_pop(&q, &sink);
        EXPECT_EQ(queue_size(&q), i - 1);
    }

    for (QUEUE_TYPE i = 0; i < capacity; i++) {
        queue_push(&q, i);
        EXPECT_EQ(queue_size(&q), i + 1);
    }

    for (QUEUE_TYPE i = capacity; i > 0; i--) {
        QUEUE_TYPE sink;
        queue_pop(&q, &sink);
        EXPECT_EQ(queue_size(&q), i - 1);
    }

    // Verify that no errors occurred
    EXPECT_EQ(errno, 0);

    queue_destroy(&q);
}
