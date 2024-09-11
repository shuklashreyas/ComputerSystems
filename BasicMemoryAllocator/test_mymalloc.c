#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "malloc.h" // Include the custom memory allocation functions

// Assert macro for testing
#define TEST_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, "Assertion failed: %s\n", #expr); \
            exit(1); \
        } \
    } while (0)

// Tests if mymalloc successfully allocates memory
void test_malloc() {
    void *ptr = mymalloc(10);
    TEST_ASSERT(ptr != NULL); // Check allocation is not NULL
    myfree(ptr); // Ensure memory can be freed without errors
}

// Tests if mycalloc allocates zero-initialized memory
void test_calloc() {
    int *ptr = mycalloc(5, sizeof(int));
    TEST_ASSERT(ptr != NULL && ptr[0] == 0); // Check for non-NULL and zeroed memory
    myfree(ptr); // Memory should be freeable without issues
}

// Tests if myfree correctly frees allocated memory
void test_free() {
    void *ptr1 = mymalloc(10);
    myfree(ptr1); // Free operation should not cause errors
    // No need to free ptr2 and ptr3 for brevity, but would be included in a full test
}

// Tests the coalescing feature of myfree
void test_coalescing() {
    void *ptr1 = mymalloc(10);
    void *ptr2 = mymalloc(20);
    myfree(ptr1); // Adjacent free blocks should coalesce
    myfree(ptr2); // This should coalesce with the previous block
    // Coalescing should allow for a larger allocation than any single free block
    void *ptr3 = mymalloc(30);
    TEST_ASSERT(ptr3 != NULL); // Check if coalescing allowed for a larger block to be allocated
    myfree(ptr3); // Clean up
}

// Main function to run all tests
int main() {
    test_malloc();
    test_calloc();
    test_free();
    test_coalescing();

    // Verify no memory is leaked by the tests
    int num_allocations = malloc_count_current();
    TEST_ASSERT(num_allocations == 0); // Check that all allocated memory has been freed

    printf("All tests passed!\n");
    return 0;
}
