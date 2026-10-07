# Add Google C++ Testing Framework as part of this project
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
FetchContent_MakeAvailable(googletest)
include(GoogleTest)



add_subdirectory(trie)

set(TEST "T-stack")
add_executable(${TEST} stack.cpp)
target_link_libraries(${TEST} PRIVATE ${CTOOLS_LIB} GTest::gtest_main)
gtest_discover_tests(${TEST})

set(TEST "T-stack_concurrency")
add_executable(${TEST} stack_concurrency.cpp)
target_link_libraries(${TEST} PRIVATE ${CTOOLS_LIB} GTest::gtest_main)
gtest_discover_tests(${TEST})

set(TEST "T-cbuf")
add_executable(${TEST} cbuf.cpp)
target_link_libraries(${TEST} PRIVATE ${CTOOLS_LIB} GTest::gtest_main)
gtest_discover_tests(${TEST})

set(TEST "T-queue")
add_executable(${TEST} queue.cpp)
target_link_libraries(${TEST} PRIVATE ${CTOOLS_LIB} GTest::gtest_main)
gtest_discover_tests(${TEST})

set(TEST "T-heap")
add_executable(${TEST} heap.cpp)
target_compile_definitions(${TEST} PRIVATE CTOOLS_ENABLE_DEBUG_ASSERT)
target_link_libraries(${TEST} PRIVATE ${CTOOLS_LIB} GTest::gtest_main)
gtest_discover_tests(${TEST})

set(TEST "T-stack_constant_capacity")
add_executable(${TEST} stack_constant_capacity.cpp)
target_link_libraries(${TEST} PRIVATE ${CTOOLS_LIB} GTest::gtest_main)
gtest_discover_tests(${TEST})

set(TEST "T-kqueue")
add_executable(${TEST} kqueue.cpp)
target_link_libraries(${TEST} PRIVATE ${CTOOLS_LIB} GTest::gtest_main)
gtest_discover_tests(${TEST})

set(TEST "T-twheel")
add_executable(${TEST} twheel.cpp)
target_link_libraries(${TEST} PRIVATE ${CTOOLS_LIB} GTest::gtest_main)
gtest_discover_tests(${TEST})

set(TEST "T-bitset")
add_executable(${TEST} bitset.cpp)
target_link_libraries(${TEST} PRIVATE ${CTOOLS_LIB} GTest::gtest_main)
gtest_discover_tests(${TEST})

set(TEST "T-rtree")
add_executable(${TEST} rtree.cpp)
target_link_libraries(${TEST} PRIVATE ${CTOOLS_LIB} GTest::gtest_main)
gtest_discover_tests(${TEST})
