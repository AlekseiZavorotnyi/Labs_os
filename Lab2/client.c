#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>


typedef struct {
    int *array;
    int ar_max;
    int ar_min;
    int start;
    int end;
} thread_data;

void* Find_min_max(void* arg) {
    thread_data *data = (thread_data *)arg;
    int *array = data->array;
    int start = data->start, end = data->end;
    int ar_max = array[start], ar_min = array[start];
    for (size_t i = start; i < end; i++) {
        if (array[i] > ar_max) {
            ar_max = array[i];
        }
        if (array[i] < ar_min) {
            ar_min = array[i];
        }
    }
    data->ar_max = ar_max;
    data->ar_min = ar_min;
    pthread_exit(NULL);
    return NULL;
}


int main(int argc, char **argv) {

    int ind_array = atoi(argv[2]);
    int *array = (int*) malloc(ind_array * sizeof (int));
    if (array == NULL) {
        const char msg[] = "error: failed to allocate memory\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }
    srand(time(NULL));
    for (int i = 0; i < ind_array; i++) {
        array[i] = rand() % 2000 - 1000;
        //printf("%d ", array[i]);
    }
    //printf("\n");
    int num_of_threads = atoi(argv[1]);

    pthread_t threads[num_of_threads];
    thread_data thread_data_array[num_of_threads];

    for (int i = 0; i < num_of_threads; i++) {
        thread_data_array[i].array = array;
        thread_data_array[i].start = ind_array / num_of_threads * i;
        if (i == num_of_threads - 1){
            thread_data_array[i].end = ind_array;
        }
        else{
            thread_data_array[i].end = thread_data_array[i].start + ind_array / num_of_threads;
        }
        if (pthread_create(&threads[i], NULL, Find_min_max, &thread_data_array[i]) != 0) {
            const char msg[] = "error: failed to create thread\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }
    }

    int arr_mx = -40000, arr_mn = 40000;
    for (int i = 0; i < num_of_threads; i++) {
        pthread_join(threads[i], NULL);
        if (arr_mx < thread_data_array[i].ar_max) {
            arr_mx = thread_data_array[i].ar_max;
        }
        if (arr_mn > thread_data_array[i].ar_min) {
            arr_mn = thread_data_array[i].ar_min;
        }
    }
    printf("Min = %d\nMax = %d\n", arr_mn, arr_mx);
    free(array);
    return 0;
}