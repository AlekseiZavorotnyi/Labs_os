#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <unistd.h>

#define MAX_BLOCK_SIZE_EXTENT 10  // Максимальный размер блока (2^10 = 1024)
#define MIN_BLOCK_SIZE_EXTENT 0    // Минимальный размер блока (2^0 = 1)
#define NUM_LISTS 11          // Количество списков (от 0 до 10)


typedef struct Block {
    struct Block *next;
    struct Block *prev;
    size_t size;
} Block;

typedef struct Allocator {
    Block* freeBlocks[NUM_LISTS];
    void *memory;
    size_t total_size;
} Allocator;

int power(int base, int exp) {
    long long result = 1;
    while (exp > 0) {
        if (exp % 2 == 1) {
            result *= base;
        }
        base *= base;
        exp /= 2;
    }
    return result;
}

Allocator* allocator_create(void* memory, size_t size) {

    Allocator* allocator = (Allocator*)memory;
    allocator->total_size = size - sizeof(Allocator);
    allocator->memory = (char*)memory + sizeof(Allocator);
    size_t offset = 0;

    size_t extent = 0;
    for (int i = 0; i < 10; ++i) {
        allocator->freeBlocks[i] = NULL;
    }
    while (offset + power(2, extent / 10) <= allocator->total_size) {
        Block *block = (Block *)((char *)allocator->memory + offset);
        if (allocator->freeBlocks[extent / 10] == NULL) {
            block->next = NULL;
            allocator->freeBlocks[extent / 10] = block;
        } else {
            allocator->freeBlocks[extent / 10]->prev = block;
        }
        block->next = allocator->freeBlocks[extent / 10];
        block->size = power(2, extent / 10);
        offset += power(2, extent / 10);
        extent++;
    }
    return allocator;
}

void split_block(Allocator *allocator, Block* block) {
    int ind = 0;
    while ((1 << ind) < block->size) {
        ind++;
    }
    Block *block_copy = (Block *)((char *)allocator->memory + block->size / 2);
    if (allocator->freeBlocks[ind] == NULL) {
        block->next = NULL;
        allocator->freeBlocks[ind] = block;
        allocator->freeBlocks[ind]->prev = block_copy;

    } else {
        allocator->freeBlocks[ind]->prev = block;
        block->next = allocator->freeBlocks[ind];
        allocator->freeBlocks[ind]->prev = block_copy;
        block_copy->next = allocator->freeBlocks[ind];
    }
    block->size = power(2, ind);
    block_copy->size = power(2, ind);
}

void* my_malloc(Allocator *allocator, size_t size) {
    int ind = 0;
    while ((1 << ind) < size) {
        ind++;
    }

    if (ind >= NUM_LISTS) return NULL;

    if (allocator->freeBlocks[ind] != NULL) {
        Block *block = allocator->freeBlocks[ind];
        allocator->freeBlocks[ind] = block->next;
        return block;
    }

    for (int i = 0; i < 10 - ind; ++i) {
        if (allocator->freeBlocks[ind + i] != NULL) {
            int j = 0;
            while (i != j) {
                Block *block = allocator->freeBlocks[ind + i - j];
                allocator->freeBlocks[ind + i - j] = block->next;
                split_block(allocator, block);
            }

            break;
        }
    }

    return NULL;
}

void my_free(Allocator *allocator, void *ptr) {

    if (allocator == NULL || ptr == NULL)
    {
        return;
    }

    int ind = 0;
    while ((1 << ind) < ((Block*)ptr)->size) {
        ind++;
    }

    allocator->freeBlocks[ind] = ((Block*)ptr)->next;
    ptr = NULL;
}

void allocator_destroy(Allocator *allocator) {
    if (!allocator)
    {
        return;
    }

    if (munmap((void *)allocator, allocator->total_size + sizeof(Allocator)) == 1)
    {
        exit(EXIT_FAILURE);
    }
}
