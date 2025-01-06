#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/mman.h>
#include <semaphore.h>

#define SHM_NAME "/Data"
#define SEM_PARENT "/parent_semaphore"
#define SEM_CHILD "/child_semaphore"

int isPrime(int N) {
    if (N == 1) {
        return 0;
    }
    if (N == 2) {
        return 1;
    }
    if (N % 2 == 0){
        return 0;
    }
    for (int i = 3; i * i <= N; i++) {
        if (N % i == 0) {
            return 0;
        }
        i++;
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s output_file\n", argv[0]);
        return EXIT_FAILURE;
    }

    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return EXIT_FAILURE;
    }

    char *ptr = (char *)mmap(NULL, BUFSIZ, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }

    sem_t *sem_parent = sem_open(SEM_PARENT, O_CREAT, 0666, 0);
    sem_t *sem_child = sem_open(SEM_CHILD, O_CREAT, 0666, 0);

    if (sem_parent == SEM_FAILED || sem_child == SEM_FAILED) {
        perror("sem_open");
        munmap(ptr, BUFSIZ);
        close(fd);
        return EXIT_FAILURE;
    }

    pid_t pid = getpid();
    int32_t file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0600);

    if (file == -1) {
        perror("open");
        sem_close(sem_parent);
        sem_close(sem_child);
        munmap(ptr, BUFSIZ);
        close(fd);
        return EXIT_FAILURE;
    }

    {
        char msg[128];
        int32_t len = snprintf(msg, sizeof(msg) - 1,
                               "%d: Start typing lines of text. Press 'Ctrl-D' with no input to exit\n", pid);
        write(STDOUT_FILENO, msg, len);
    }

    char buf[BUFSIZ];
    ssize_t bytes;
    char ans[BUFSIZ];

    while (1) {

        sem_wait(sem_child);

        strncpy(buf, ptr, BUFSIZ);
        if (strlen(buf) == 0) {
            break;
        }

        if (!isPrime(atoi(buf))) {
            snprintf(ans, sizeof(ans), "%s\n", buf);
            write(file, ans, strlen(ans));
        } else {
            if (strlen(buf)) {
                snprintf(ans, sizeof(ans), "Prime number or invalid str: %s\n", buf);
                write(STDOUT_FILENO, ans, strlen(ans));
                sem_post(sem_parent);
                break;
            }
        }

        sem_post(sem_parent);
    }

    munmap(ptr, BUFSIZ);
    close(fd);
    sem_close(sem_parent);
    sem_close(sem_child);
    close(file);
}