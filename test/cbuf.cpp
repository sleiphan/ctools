#include <gtest/gtest.h>
#include <stdio.h>

extern "C" {
#include "ctools/cbuf.h"
}

TEST(cbuf, trigger_rollover) {
    // Chooses an amount of integers and transfer size that
    // is intentionally non-aligned with page sizes.

    const unsigned int int_count = 10000;
    const unsigned int transfer_count = 1000;

    const unsigned int transfer_count_b = transfer_count * sizeof(unsigned int);
    unsigned int expected[int_count], actual[transfer_count];

    memset(actual, -1, sizeof(actual));

    for (unsigned int i = 0; i < int_count; i++)
        expected[i] = i;

    struct ct_cbuf circular_buffer;
    int res = ct_cbuf_init(&circular_buffer, 1);
    EXPECT_EQ(res, 0);

    for (unsigned int i = 0; i < int_count; i += transfer_count) {
        res = ct_cbuf_write(&circular_buffer, &expected[i], transfer_count_b);
        EXPECT_EQ(res, transfer_count_b);

        res = ct_cbuf_read(&circular_buffer, actual, transfer_count_b);
        EXPECT_EQ(res, transfer_count_b);

        // Verify that the read values are the same as the values written
        for (unsigned int y = 0; y < transfer_count; y++)
            EXPECT_EQ(expected[i + y], actual[y]);
    }

    ct_cbuf_exit(&circular_buffer);
}
