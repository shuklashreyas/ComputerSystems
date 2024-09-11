#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define BLOCK_SIZE sizeof(block_t)

typedef struct block {
    size_t size;
    struct block *next;
    bool free;
} block_t;

block_t *head = NULL;

// Debug print statements
#define debug_printf(...) fprintf(stderr, __VA_ARGS__)

// Global variables to track total bytes allocated and freed
size_t total_allocated = 0;
size_t total_freed = 0;

void *mymalloc(size_t size) {
    debug_printf("Malloc %zu bytes\n", size);  // Debug statement for malloc

    if (size <= 0) {
        return NULL;
    }

    block_t *current = head;
    block_t *prev = NULL;
    while (current != NULL) {
        if (current->free && current->size >= size) {
            if (current->size >= size + BLOCK_SIZE + sizeof(block_t)) {
                // Splitting block
                block_t *new_block = (block_t *)((char *)current + BLOCK_SIZE + size);
                new_block->size = current->size - size - BLOCK_SIZE;
                new_block->next = current->next;
                new_block->free = true;

                current->size = size;
                current->next = new_block;
            }
            current->free = false;
            total_allocated += size;
            return (void *)(current + 1);
        }
        prev = current;
        current = current->next;
    }

    // No free block found, allocate new block
    block_t *new_block = (block_t *)sbrk(BLOCK_SIZE + size);
    if (new_block == (void *)-1) {
        return NULL;
    }
    new_block->size = size;
    new_block->next = NULL;
    new_block->free = false;
    
    if (head == NULL) {
        head = new_block;
    } else {
        prev->next = new_block;
    }

    total_allocated += size;
    return (void *)(new_block + 1);
}

void *mycalloc(size_t nmemb, size_t size) {
    debug_printf("Calloc %zu bytes\n", nmemb * size);  // Debug statement for calloc

    size_t total_size = nmemb * size;
    void *ptr = mymalloc(total_size);
    if (ptr == NULL) {
        return NULL;
    }
    memset(ptr, 0, total_size);
    return ptr;
}

void myfree(void *ptr) {
    if (ptr == NULL) {
        debug_printf("Attempt to free a null pointer\n");
        return;
    }

    block_t *block_to_free = (block_t *)ptr - 1;
    if (block_to_free->free) {
        debug_printf("Attempt to free an already free block\n");
        return;
    }

    // Mark the block as free
    block_to_free->free = true;
    total_freed += block_to_free->size;
    debug_printf("Freed %zu bytes; total freed now %zu bytes\n", block_to_free->size, total_freed);

    // Coalesce free blocks
    block_t *current = head;
    while (current) {
        // If the current block is free and the next block is free, merge them
        if (current->free && current->next && current->next->free) {
            current->size += BLOCK_SIZE + current->next->size;
            current->next = current->next->next;
            debug_printf("Coalesced with next block, new size %zu bytes\n", current->size);
        }
        current = current->next;
    }
}


// Function to return the number of active allocations
size_t malloc_count_current() {
    size_t num_allocations = 0;
    block_t *current = head;
    while (current) {
        if (!current->free) {
            num_allocations++;
        }
        current = current->next;
    }
    return (head == NULL) ? 0 : num_allocations;
}
