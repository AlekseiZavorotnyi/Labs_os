#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

typedef struct Block {
    struct Block* next;
    size_t size;
} Block;

typedef struct Allocator {
    Block* freeBlocks;
    void* memory;
    size_t memory_size;
} Allocator;

Allocator* allocator_create(void* const memory, const size_t size) {
    if (!memory || size < sizeof(Block)) {
        return NULL;
    }
    Allocator* allocator = (Allocator*)memory;
    allocator->memory_size = size - sizeof(Allocator);
    allocator->memory = (char*)memory + sizeof(Allocator);
    allocator->freeBlocks = allocator->memory;
    Block* initial_block = (Block*)allocator->memory;
    initial_block->size = allocator->memory_size;
    initial_block->next = NULL;
    return allocator;
}

void* my_malloc(Allocator* const allocator, const size_t size) {
    if (!allocator || size == 0) {
        return NULL;
    }

    Block* prev = NULL;
    Block* curr = (Block*)allocator->freeBlocks;

    while (curr) {
        if (curr->size >= size + sizeof(Block)) {
            if (curr->size > size + sizeof(Block)) {
                Block* new_block = (Block*)((char*)curr + sizeof(Block) + size);
                new_block->size = curr->size - size - sizeof(Block);
                new_block->next = curr->next;

                curr->size = size;
                curr->next = new_block;
            }

            if (prev) {
                prev->next = curr->next;
            } else {
                allocator->freeBlocks = curr->next;
            }

            return (char*)curr + sizeof(Block);
        }

        prev = curr;
        curr = curr->next;
    }

    return NULL;
}

void my_free(Allocator* const allocator, void* const memory) {
    if (!allocator || !memory) {
        return;
    }

    Block* block = (Block*)((char*)memory - sizeof(Block));
    block->next = (Block*)allocator->freeBlocks;
    allocator->freeBlocks = block;
}

void allocator_destroy(Allocator* const allocator) {
    allocator->memory = NULL;
    allocator->memory_size = 0;
    allocator->freeBlocks = NULL;
}