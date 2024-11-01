#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>

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

int Validatenum(const char* argv) {
    int cnt = 0;
    while (*argv != '\0' && *argv != '\n') {
        if (!isdigit(*argv)) {
            return 0;
        }
        cnt++;
        argv++;
    }
    if (cnt > 8){
        return -1;
    }
    return 1;
}

int main(int argc, char **argv) {
    char buf[4096];
    ssize_t bytes;
    pid_t pid = getpid();
    int32_t file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0600);
    if (file == -1) {
        const char msg[] = "error: failed to open requested file\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    {
        char msg[128];
        int32_t len = snprintf(msg, sizeof(msg) - 1,
            "%d: Start typing not prime numbers. Type prime number or press 'Ctrl-D' or 'Enter' with no input to exit\n", pid);
        write(STDOUT_FILENO, msg, len);
    }

    while (bytes = read(STDIN_FILENO, buf, sizeof(buf))) {
        if (bytes < 0) {
            const char msg[] = "error: failed to read from stdin\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        } else if (buf[0] == '\n') {
            break;
        }
        {
            buf[bytes - 1] = '\0';
            switch(Validatenum(buf)){
                case 0:
                    const char msg[] = "error: one of arguments is not a number\n";
                    write(STDERR_FILENO, msg, sizeof(msg));
                    exit(EXIT_FAILURE);
                case -1:
                    const char msg1[] = "error: too many symbols\n";
                    write(STDERR_FILENO, msg1, sizeof(msg1));
                    exit(EXIT_FAILURE);
                default: break;
            }
            int num = atoi(buf);
            if (isPrime(num) == 1) {
                const char msg[] = "error: one of numbers is prime\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            }
            buf[bytes - 1] = '\n';
            int32_t written = write(file, buf, bytes);
            if (written != bytes) {
                const char msg[] = "error: failed to write to file\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            }
        }
    }
    const char term = '\0';
    write(file, &term, sizeof(term));
    close(file);
}
