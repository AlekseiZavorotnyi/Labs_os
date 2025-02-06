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
    if (memory == NULL || size < sizeof(Block)) {
        return NULL;
    }
    Allocator* allocator = memory;
    allocator->memory_size = size - sizeof(Allocator);
    allocator->memory = (char*)memory + sizeof(Allocator);
    allocator->freeBlocks = allocator->memory;
    Block* all_memory_block = allocator->memory;
    all_memory_block->size = allocator->memory_size;
    all_memory_block->next = NULL;
    return allocator;
}

void* my_malloc(Allocator* const allocator, const size_t size) {
    if (allocator == NULL || size == 0) {
        return NULL;
    }
    Block* previous_block = NULL, *current_block = allocator->freeBlocks;
    while (current_block != NULL) {
        if (current_block->size >= size + sizeof(Block)) {
            if (current_block->size > size + sizeof(Block)) {
                Block* new_block = (Block*)((char*)current_block + sizeof(Block) + size);
                new_block->size = current_block->size - size - sizeof(Block);
                new_block->next = current_block->next;

                current_block->size = size;
                current_block->next = new_block;
            }
            if (previous_block != NULL) {
                previous_block->next = current_block->next;
            } else {
                allocator->freeBlocks = current_block->next;
            }
            return (char*)current_block + sizeof(Block);
        }
        previous_block = current_block;
        current_block = current_block->next;
    }

    return NULL;
}

void my_free(Allocator* const allocator, void* const memory) {
    if (allocator == NULL || memory == NULL) {return;}
    Block* block = (Block*)((char*)memory - sizeof(Block));
    block->next = allocator->freeBlocks;
    allocator->freeBlocks = block;
}

void allocator_destroy(Allocator* const allocator) {
    allocator->memory = NULL;
    allocator->memory_size = 0;
    allocator->freeBlocks = NULL;
}