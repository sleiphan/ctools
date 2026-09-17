#include <algorithm>
#include <gtest/gtest.h>
#include <random>

#define TWHEEL_NAME twheel
#define TWHEEL_INDEX unsigned int
#define TWHEEL_TYPE unsigned int
extern "C" {
#include "ctools/twheel.h"
}

TEST(twheel, basic_usage) {
    struct twheel tw;
    EXPECT_EQ(twheel_create(&tw, 1, 5, 8), 0);

    for (int i = 1; i < 4; i++)
        EXPECT_EQ(twheel_schedule(&tw, i, i, NULL), 0);

    EXPECT_EQ(twheel_advance(&tw, 4), 3);

    for (int i = 1; i < 4; i++) {
        unsigned int out_value = -1;
        EXPECT_EQ(twheel_pop(&tw, &out_value), 0);
        EXPECT_EQ(out_value, i);
    }

    twheel_destroy(&tw);
}

TEST(twheel, error_when_popping_empty) {
    struct twheel tw;
    unsigned int out_value = -1;
    EXPECT_EQ(twheel_create(&tw, 1, 5, 8), 0);
    EXPECT_EQ(twheel_schedule(&tw, 1, 1, NULL), 0);
    EXPECT_EQ(twheel_advance(&tw, 2), 1);
    EXPECT_EQ(twheel_pop(&tw, &out_value), 0);
    EXPECT_EQ(out_value, 1);
    EXPECT_EQ(twheel_pop(&tw, &out_value), -1);

    twheel_destroy(&tw);
}

TEST(twheel, roundtrip_through_buckets) {
    struct twheel tw;
    unsigned int out_value = -1;
    EXPECT_EQ(twheel_create(&tw, 1, 4, 8), 0);
    EXPECT_EQ(twheel_advance(&tw, 2), 0);
    EXPECT_EQ(twheel_schedule(&tw, 2, 1, NULL), 0);
    EXPECT_EQ(twheel_advance(&tw, 4), 1);
    EXPECT_EQ(twheel_pop(&tw, &out_value), 0);
    EXPECT_EQ(out_value, 1);

    twheel_destroy(&tw);
}

TEST(twheel, FIFO_respected) {
    struct twheel tw;
    EXPECT_EQ(twheel_create(&tw, 1, 4, 8), 0);

    for (int i = 0; i < 4; i++)
        EXPECT_EQ(twheel_schedule(&tw, 1, i, NULL), 0);

    EXPECT_EQ(twheel_advance(&tw, 2), 1);

    for (int i = 0; i < 4; i++) {
        unsigned int out_value = -1;
        EXPECT_EQ(twheel_pop(&tw, &out_value), 0);
        EXPECT_EQ(out_value, i);
    }

    twheel_destroy(&tw);
}

TEST(twheel, advancing_beyond_bucket_count) {
    const unsigned int bucket_count = 4;

    struct twheel tw;
    EXPECT_EQ(twheel_create(&tw, 1, bucket_count, 8), 0);

    for (int i = 1; i < bucket_count; i++)
        EXPECT_EQ(twheel_schedule(&tw, i, i, NULL), 0);

    EXPECT_EQ(twheel_advance(&tw, bucket_count * 3 / 2), bucket_count - 1);

    for (int i = 1; i < bucket_count; i++) {
        unsigned int out_value = -1;
        EXPECT_EQ(twheel_pop(&tw, &out_value), 0);
        EXPECT_EQ(out_value, i);
    }

    twheel_destroy(&tw);
}

TEST(twheel, cancelling_base_case) {
    const unsigned int bucket_count = 4;

    struct twheel tw;
    EXPECT_EQ(twheel_create(&tw, 1, bucket_count, bucket_count * 2), 0);

    struct twheel_handle timer;

    for (int i = 1; i < bucket_count; i++)
        EXPECT_EQ(twheel_schedule(&tw, i, i, &timer), 0);

    EXPECT_EQ(twheel_cancel(&tw, timer), 0);

    EXPECT_EQ(twheel_advance(&tw, bucket_count * 3 / 2), bucket_count - 2);

    for (int i = 1; i < bucket_count - 1; i++) {
        unsigned int out_value = -1;
        EXPECT_EQ(twheel_pop(&tw, &out_value), 0);
        EXPECT_EQ(out_value, i);
    }

    twheel_destroy(&tw);
}

TEST(twheel, webserver_case) {
    // One in timeout_client_for_every client will time out.
    const TWHEEL_INDEX timeout_client_for_every = 10'000;

    const TWHEEL_INDEX timer_count = 1'000'000;
    const TWHEEL_INDEX time_span_ms = 10'000;
    const TWHEEL_INDEX interval_ms = 100;
    struct twheel_handle *handles =
        (struct twheel_handle *)malloc(timer_count * sizeof(struct twheel_handle));
    TWHEEL_INDEX handles_count = 0;

    TWHEEL_INDEX *timed_out_clients = (TWHEEL_INDEX *)malloc(timer_count * sizeof(TWHEEL_INDEX));
    TWHEEL_INDEX timed_out_clients_count = 0;

    struct twheel tw;
    EXPECT_EQ(twheel_create(&tw, interval_ms, time_span_ms / interval_ms, timer_count), 0);

    for (TWHEEL_INDEX i = 0; i < timer_count; i++) {
        // Choose a random timeout, simulating clients having connected
        // at different times.
        const TWHEEL_INDEX min = 1, max = tw.max_timeout;
        TWHEEL_TICK timeout = (rand() % (max - min)) + min;

        // Choose clients that will time out.
        bool will_time_out = (rand() % timeout_client_for_every) == 1;
        // bool will_time_out = true;
        struct twheel_handle *handle = will_time_out ? NULL : &handles[handles_count++];
        timed_out_clients[timed_out_clients_count] = i;
        timed_out_clients_count += will_time_out;

        int err = twheel_schedule(&tw, timeout, i, handle);
        if (err)
            FAIL();
        EXPECT_EQ(err, 0);
    }

    // Shuffle timer handles.
    // This simulates clients sending data to the server in random order,
    // making the server cancel the timeouts in random order.
    std::mt19937 rng{std::random_device{}()};
    std::shuffle(&handles[0], &handles[handles_count], rng);

    // Cancel timers for clients that won't time out
    for (int i = 0; i < handles_count; i++)
        EXPECT_EQ(twheel_cancel(&tw, handles[i]), 0);

    // Time out all remaining timers
    int err = twheel_advance(&tw, tw.max_timeout + 1);
    EXPECT_GT(err, 0);

    // Pop all timed out timers
    TWHEEL_TYPE *out_values = (TWHEEL_TYPE *)malloc(timed_out_clients_count * sizeof(TWHEEL_TYPE));
    TWHEEL_TYPE pop_idx = 0;
    for (; !twheel_pop(&tw, &out_values[pop_idx]); pop_idx++)
        ;

    std::sort(out_values, &out_values[timed_out_clients_count]);
    std::sort(timed_out_clients, &timed_out_clients[timed_out_clients_count]);

    for (TWHEEL_INDEX i = 0; i < timed_out_clients_count; i++)
        EXPECT_EQ(out_values[i], timed_out_clients[i]);

    EXPECT_EQ(twheel_is_empty(&tw), 1);

    twheel_destroy(&tw);
    free(timed_out_clients);
    free(handles);
}

TEST(twheel, wait_returns_closest_timeout) {
    const unsigned int time_span_ms = 40000;
    const unsigned int interval_ms = 100;
    const unsigned int bucket_count = time_span_ms / interval_ms;
    const unsigned int timeout_1_ms = 300;

    struct twheel tw;
    EXPECT_EQ(twheel_create(&tw, interval_ms, bucket_count, bucket_count * 2), 0);

    EXPECT_EQ(twheel_schedule(&tw, timeout_1_ms, 1, NULL), 0);
    EXPECT_EQ(twheel_schedule(&tw, timeout_1_ms * 2, 2, NULL), 0);

    TWHEEL_TICK time = 0;
    EXPECT_EQ(twheel_wait(&tw, &time), 0);
    EXPECT_EQ(time, timeout_1_ms);

    twheel_destroy(&tw);
}

TEST(twheel, wait_returns_closest_timeout_after_cancel) {
    const unsigned int time_span_ms = 40000;
    const unsigned int interval_ms = 100;
    const unsigned int bucket_count = time_span_ms / interval_ms;
    const unsigned int timeout_2_ms = 400;

    struct twheel tw;
    EXPECT_EQ(twheel_create(&tw, interval_ms, bucket_count, bucket_count * 2), 0);

    struct twheel_handle handle;

    EXPECT_EQ(twheel_schedule(&tw, timeout_2_ms / 2, 1, &handle), 0);
    EXPECT_EQ(twheel_schedule(&tw, timeout_2_ms, 2, NULL), 0);

    EXPECT_EQ(twheel_cancel(&tw, handle), 0);

    TWHEEL_TICK time = 0;
    EXPECT_EQ(twheel_wait(&tw, &time), 0);
    EXPECT_EQ(time, timeout_2_ms);

    twheel_destroy(&tw);
}

TEST(twheel, wait_returns_closest_timeout_wrapped) {
    const unsigned int time_span_ms = 40000;
    const unsigned int interval_ms = 100;
    const unsigned int bucket_count = time_span_ms / interval_ms;
    const unsigned int timeout_ms = time_span_ms * 3 / 4;

    struct twheel tw;
    EXPECT_EQ(twheel_create(&tw, interval_ms, bucket_count, bucket_count * 2), 0);

    // Advance time wheel halfway through the wheel
    EXPECT_EQ(twheel_advance(&tw, time_span_ms / 2), 0);

    // Add a timer that wraps past the end of the wheel
    EXPECT_EQ(twheel_schedule(&tw, timeout_ms, 1, NULL), 0);

    TWHEEL_TICK time = 0;
    EXPECT_EQ(twheel_wait(&tw, &time), 0);
    EXPECT_EQ(time, timeout_ms);

    twheel_destroy(&tw);
}
