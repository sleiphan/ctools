#include <algorithm>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

#define STACK_EXT_DYNAMIC_SIZE
#define STACK_NAME stack
#define STACK_TYPE int
#define STACK_EXT_THREAD_SAFE
extern "C" {
#include "ctools/stack.h"
}

void speed_difference_test(const int producer_delay_ms, const int consumer_delay_ms) {
    stack s;
    stack_create(&s, 0);

    const int NUMBER_COUNT = 300;
    int producer_numbers[NUMBER_COUNT];
    int consumed_numbers[NUMBER_COUNT];

    for (int i = 0; i < NUMBER_COUNT; i++)
        producer_numbers[i] = i;

    std::thread producer([&s, &producer_numbers, &producer_delay_ms]() {
        for (int i = 0; i < NUMBER_COUNT; i++) {
            stack_push(&s, producer_numbers[i]);
            std::this_thread::sleep_for(std::chrono::milliseconds(producer_delay_ms));
        }
    });

    std::thread consumer([&s, &consumed_numbers, &consumer_delay_ms]() {
        for (int i = 0; i < NUMBER_COUNT; i++) {
            while (stack_pop(&s, &consumed_numbers[i]) != 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(consumer_delay_ms));
        }
    });

    producer.join();
    consumer.join();

    std::sort(consumed_numbers, &consumed_numbers[NUMBER_COUNT]);

    for (int i = 0; i < NUMBER_COUNT; i++)
        EXPECT_EQ(consumed_numbers[i], i);

    // Cleanup
    stack_destroy(&s);
}

TEST(stack_concurrency, faster_producer) { speed_difference_test(1, 2); }

TEST(stack_concurrency, faster_consumer) { speed_difference_test(2, 1); }
