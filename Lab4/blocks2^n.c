#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

#define MIN_SIZE_BLOCK 5
#define NUM_LISTS 21


typedef struct Block {
    struct Block *next;
    size_t size;
} Block;

typedef struct Allocator {
    Block* freeBlocks[NUM_LISTS];
    void *memory;
    size_t memory_size;
} Allocator;

int two_at_n_degrees(size_t n) {
    int result = 1, base = 2;
    while (n > 0) {
        if (n % 2 == 1) {
            result *= base;
        }
        base *= base;
        n /= 2;
    }
    return result;
}

int log_2_n(size_t size) {
    int ind = 0;
    while (1 << ind < size) {
        ind++;
    }
    return ind;
}

Allocator* allocator_create(void* memory, size_t size) {
	if (!memory || size < sizeof(Block)) {
        return NULL;
    }
    Allocator* allocator = (Allocator*)memory;
    allocator->memory_size = size - sizeof(Allocator);
    allocator->memory = (char*)memory + sizeof(Allocator);
    size_t offset = 0, power = 0;
    for (int i = 0; i < 21; i++) {
        allocator->freeBlocks[i] = NULL;
    }
    for (int i = 5; i < 21; i++) {
        size_t cur_block_size = two_at_n_degrees(power);
        for (int j = 0; j < 10; j++) {
            if (offset + two_at_n_degrees(power) > allocator->memory_size) {
                return allocator;
            }
            Block *block = (Block *)((char *)allocator->memory + offset);
            if (allocator->freeBlocks[power] == NULL) {
                block->next = NULL;
            } else {
                block->next = allocator->freeBlocks[power];
            }
            allocator->freeBlocks[power] = block;
            block->size = cur_block_size;
            offset += cur_block_size;
        }
        power++;
    }
    return allocator;
}

void split_block(Allocator *allocator, Block* block) {
    int ind = log_2_n(block->size), new_ind = ind - 1;
    allocator->freeBlocks[ind] = block->next;
    Block *second_block = (Block *)((char *)block + block->size / 2);
    allocator->freeBlocks[new_ind] = second_block;
    second_block->next = block;
    block->next = NULL;
    block->size = two_at_n_degrees(new_ind);
    second_block->size = two_at_n_degrees(new_ind);
}

void* my_malloc(Allocator *allocator, size_t size) {
    int ind = log_2_n(size);
    if (ind >= NUM_LISTS || ind < MIN_SIZE_BLOCK) return NULL;

    if (allocator->freeBlocks[ind] != NULL) {
        Block *block = allocator->freeBlocks[ind];
        allocator->freeBlocks[ind] = block->next;
        block->next = NULL;
        return block;
    }
    for (int i = 1; i < 21 - ind; i++) {
        if (allocator->freeBlocks[ind + i] != NULL) {
            int j = 0;
            while (i != j) {
                Block *block = allocator->freeBlocks[ind + i - j];
                allocator->freeBlocks[ind + i - j] = block->next;
                split_block(allocator, block);
                j++;
            }
            Block *res_block = allocator->freeBlocks[ind];
            allocator->freeBlocks[ind] = res_block->next;
            res_block->next = NULL;
            return res_block;
        }
    }
    return NULL;
}

/*void my_free(Allocator *allocator, void *ptr) {
    if (allocator == NULL || ptr == NULL)
    {
        return;
    }
	Block* block = (Block*)ptr;
    int ind = log_2_n(block->size);
    block->next = allocator->freeBlocks[ind];
    allocator->freeBlocks[ind] = block;
}*/

void my_free(Allocator* const allocator, void* const memory) {
    if (!allocator || !memory) {
        return;
    }

    Block* block = (Block*)((char*)memory - sizeof(Block));
    int ind = log_2_n(block->size);
    block->next = allocator->freeBlocks[ind];
    allocator->freeBlocks[ind] = block;
}

void allocator_destroy(Allocator *allocator) {
    allocator->memory = NULL;
    allocator->memory_size = 0;
    for (int i = 5; i < 21; i++) {
    	allocator->freeBlocks[i] = NULL;
	}
    if (munmap((void *)allocator, allocator->memory_size + sizeof(Allocator)) == 1)
    {
        exit(EXIT_FAILURE);
    }
}
