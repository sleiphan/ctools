#include <gtest/gtest.h>

#define STACK_NAME stack
#define STACK_TYPE int
extern "C" {
    #include "ctools/stack.h"
}

TEST(stack, elements_are_popped_in_the_correct_order) {
    stack s;
    stack_create(&s, 0);

    const int NUM_VALUES = 10;
    int actual[NUM_VALUES];

    // Insert values into the stack in descending order
    for (int i = NUM_VALUES - 1; i >= 0; i--)
        stack_push(&s, i);

    // Read from the stack into 'actual'
    for (int i = 0; i < NUM_VALUES; i++)
        stack_pop(&s, &actual[i]);
    
    // Verify
    for (int i = 0; i < NUM_VALUES; i++)
        EXPECT_EQ(i, actual[i]);
    
    // Cleanup
    stack_destroy(&s);
}

TEST(stack, clear_function_removes_all_entries) {
    stack s;
    stack_create(&s, 0);

    const int VALUE = 42;
    const int ELEMENTS_TO_PUSH = 10;

    int err = -1;
    for (int i = 0; i < ELEMENTS_TO_PUSH; i++) {
        err = stack_push(&s, VALUE);
        EXPECT_EQ(err, 0);
    }

    int size = stack_size(&s);
    EXPECT_EQ(size, ELEMENTS_TO_PUSH);

    err = stack_clear(&s);
    EXPECT_EQ(err, 0);

    size = stack_size(&s);
    EXPECT_EQ(size, 0);
    
    // Cleanup
    stack_destroy(&s);
}

TEST(stack, clearing_an_empty_stack) {
    stack s;
    stack_create(&s, 0);

    int size = stack_size(&s);
    EXPECT_EQ(size, 0);

    int err = stack_clear(&s);
    EXPECT_EQ(err, 0);

    size = stack_size(&s);
    EXPECT_EQ(size, 0);

    // Cleanup
    stack_destroy(&s);
}
