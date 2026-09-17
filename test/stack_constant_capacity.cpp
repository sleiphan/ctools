#include <gtest/gtest.h>

#define STACK_NAME stack
#define STACK_TYPE int
extern "C" {
#include "ctools/stack.h"
}

TEST(stack, errors_when_overflowing) {
    const unsigned int cap = 8;
    stack s;
    stack_create(&s, cap);

    for (int i = 0; i < cap; i++)
        EXPECT_EQ(stack_push(&s, i), 0);

    EXPECT_EQ(stack_push(&s, cap), -1);
    stack_destroy(&s);
}
