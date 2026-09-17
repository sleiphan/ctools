#include <gtest/gtest.h>
#include <stdio.h>

#define HEAP_NAME heap
#define HEAP_TYPE int
#define HEAP_COMP(a, b) b - a

extern "C" {
#include "ctools/heap.h"
}

TEST(heap_verify, base_case) {
    const int heap_array[] = {9, 8, 7, 6, 5, 4, 3, 2, 1};

    EXPECT_EQ(heap_verify(heap_array, 9), 0);
}

TEST(heap_verify, even_array_size) {
    const int heap_array[] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

    EXPECT_EQ(heap_verify(heap_array, 10), 0);
}

TEST(heap_verify, swapped_largest_child) {
    const int heap_array[] = {9, 7, 8, 5, 6, 3, 4, 1, 2};

    EXPECT_EQ(heap_verify(heap_array, 9), 0);
}

TEST(heap_verify, voilation_counts) {
    const int heap_array_1_violations[] = {8, 9, 7, 6, 5, 4, 3, 2, 1};
    const int heap_array_2_violations[] = {8, 9, 4, 6, 5, 7, 3, 2, 1};
    const int heap_array_3_violations[] = {8, 9, 4, 2, 5, 7, 3, 6, 1};

    EXPECT_EQ(heap_verify(heap_array_1_violations, 9), 1);
    EXPECT_EQ(heap_verify(heap_array_2_violations, 9), 2);
    EXPECT_EQ(heap_verify(heap_array_3_violations, 9), 3);
}

TEST(heap_build, base_case) {
    int heap_array[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    EXPECT_EQ(heap_build(heap_array, 9), 0);

    EXPECT_EQ(heap_verify(heap_array, 9), 0);
}

TEST(heap_build, even_array_size) {
    int heap_array[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    EXPECT_EQ(heap_build(heap_array, 10), 0);

    EXPECT_EQ(heap_verify(heap_array, 9), 0);
}

TEST(heap, heap_sort_case_100k_nodes) {
    heap *h = heap_create(16);

    const int node_count = 100'000;

    // 268'435'456
    // 33'554'432
    // 10'000'000

    // Node-count: 100'000'000
    // Push-time:   16.496'680 sec
    //  Pop-time:   21.756'623 sec

    // Node-count:  10'000'000
    // Push-time:    1.507'748 sec
    //  Pop-time:    1.940'994 sec

    // Node-count:   1'000'000
    // Push-time:    0.139'017 sec
    //  Pop-time:    0.172'655 sec

    clock_t start = clock(), diff;

    // Insert values in ascending order
    for (int i = 0; i < node_count; i++)
        heap_push(h, i);

    diff = clock() - start;
    int usec = diff * (1000000 / CLOCKS_PER_SEC);
    printf("Push-time: %d usec\n", usec);

    int *results = new int[node_count];
    start = clock();

    // Pop values, expecting them to come out in ascending order
    for (int i = 0; i < node_count; i++)
        heap_pop(h, &results[i]);

    diff = clock() - start;
    usec = diff * (1000000 / CLOCKS_PER_SEC);
    printf(" Pop-time: %d usec\n", usec);

    // Validate
    for (int i = 0; i < node_count; i++)
        EXPECT_EQ(i, results[node_count - 1 - i]);

    delete[] results;
    heap_destroy(h);
}

// 0x7ffff5645030
// 0x7ffff5645030
