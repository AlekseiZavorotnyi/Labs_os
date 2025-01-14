#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>

typedef struct Allocator {
    void *(*allocator_create)(void *addr, size_t size);
    void *(*my_malloc)(void *allocator, size_t size);
    void (*my_free)(void *allocator, void *ptr);
    void (*allocator_destroy)(void *allocator);
} Allocator;

long get_current_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000000000) + ts.tv_nsec;
}

void *default_allocator_create(void *memory, size_t size) {
    (void)size;
    (void)memory;
    return memory;
}

void *default_my_malloc(void *allocator, size_t size) {
    uint32_t *memory = mmap(NULL, size + sizeof(uint32_t), PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        return NULL;
    }
    *memory = (uint32_t)(size + sizeof(uint32_t));
    return memory + 1;
}

void default_my_free(void *allocator, void *memory) {
    if (memory == NULL)
        return;
    uint32_t *mem = (uint32_t *)memory - 1;
    munmap(mem, *mem);
}

void default_allocator_destroy(void *allocator) {
    (void)allocator;
}

Allocator *load_allocator(const char *library_path) {

    if (library_path == NULL || library_path[0] == '\0') {
        char message[] = "WARNING: failed to load library (default allocator will be used)\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);

        Allocator *allocator = malloc(sizeof(Allocator));
        allocator->allocator_create = default_allocator_create;
        allocator->my_malloc = default_my_malloc;
        allocator->my_free = default_my_free;
        allocator->allocator_destroy = default_allocator_destroy;
        return allocator;
    }

    void *library = dlopen(library_path, RTLD_LOCAL | RTLD_NOW);
    if (!library) {
        char message[] = "WARNING: failed to load library\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);

        Allocator *allocator = malloc(sizeof(Allocator));
        allocator->allocator_create = default_allocator_create;
        allocator->my_malloc = default_my_malloc;
        allocator->my_free = default_my_free;
        allocator->allocator_destroy = default_allocator_destroy;
        return allocator;
    }

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "SUCCESS: allocator loaded from \'%s\'\n", library_path);
    write(STDOUT_FILENO, buffer, strlen(buffer));

    Allocator *allocator = malloc(sizeof(Allocator));
    allocator->allocator_create = dlsym(library, "allocator_create");
    allocator->my_malloc = dlsym(library, "my_malloc");
    allocator->my_free = dlsym(library, "my_free");
    allocator->allocator_destroy = dlsym(library, "allocator_destroy");
    if (!allocator->allocator_create || !allocator->my_malloc || !allocator->my_free || !allocator->allocator_destroy) {
        const char msg[] = "ERROR: failed to load all allocator functions\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        free(allocator);
        dlclose(library);
        return NULL;
    }

    return allocator;
}

int test_allocator(const char *library_path) {

  	srand(time(NULL));

    Allocator *allocator_api = load_allocator(library_path);

    if (!allocator_api) return -1;

    size_t size = 1024 * 32;
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
        char message[] = "ERROR: mmap failed\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        free(allocator_api);
        return EXIT_FAILURE;
    }

    void *allocator = allocator_api->allocator_create(addr, size);

    if (!allocator) {
        char message[] = "ERROR: failed to initialize allocator\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        munmap(addr, size);
        free(allocator_api);
        return EXIT_FAILURE;
    }

    char start_message[] = "=============Allocator initialized=============\n";
    write(STDOUT_FILENO, start_message, sizeof(start_message) - 1);
    long double sum_allocating_time = 0.0, sum_freeing_time = 0.0, start_time, end_time;

    int num_alloc, start_alloc = 0, num_free, start_free = 0, tek_num_alloc_need = 100, tek_num_free_can = 0;
	void * list_allocated_memory[100];
    while (1){
        if (tek_num_alloc_need != 0){
            num_alloc = rand() % tek_num_alloc_need;
            if (num_alloc == 0){
                num_alloc = 1;
            }
            for (int i = start_alloc; i < start_alloc + num_alloc; i++){
                start_time = get_current_time_ns();
                list_allocated_memory[i] = allocator_api->my_malloc(allocator, 3);
                end_time = get_current_time_ns();
                sum_allocating_time += (end_time - start_time);
                if (list_allocated_memory[i] == NULL) {
                    char alloc_fail_message[] = "ERROR: memory allocation failed\n";
                    write(STDERR_FILENO, alloc_fail_message, sizeof(alloc_fail_message) - 1);
                    return EXIT_FAILURE;
                }
            }
            start_alloc += num_alloc;
            tek_num_alloc_need -= num_alloc;
            tek_num_free_can += num_alloc;
        }
        if (tek_num_free_can != 0){
            num_free = rand() % tek_num_free_can;
            if (num_free == 0){
                num_free = 1;
            }
            for (int i = start_free; i < start_free + num_free; i++){
                start_time = get_current_time_ns();
                allocator_api->my_free(allocator, list_allocated_memory[i]);
                end_time = get_current_time_ns();
                sum_freeing_time += (end_time - start_time);
            }
            start_free += num_free;
            tek_num_free_can -= num_free;
        }
        else{
            break;
        }
    }

    sum_allocating_time /= 1000000.0;
    sum_freeing_time /= 1000000.0;

    char time_message[100];
    snprintf(time_message, sizeof(time_message), "Allocation took %Lf ms and freeing took %Lf ms.\n", sum_allocating_time / 100.0, sum_freeing_time / 100.0);
    write(STDOUT_FILENO, time_message, strlen(time_message));

    allocator_api->allocator_destroy(allocator);
    free(allocator_api);
    munmap(addr, size);

    char exit_message[] = "=============Allocator destroyed===============\n";

    write(STDOUT_FILENO, exit_message, sizeof(exit_message) - 1);

    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    const char *library_path = (argc > 1) ? argv[1] : NULL;

    if (test_allocator(library_path))
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}