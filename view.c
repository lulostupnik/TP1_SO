#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include "view.h"

#define SHM_SIZE 4096
#define BUFFER_SIZE 256

size_t readShm(char *buffer, shmSegment *segment, size_t maxBytes) {
    size_t bytesRead = 0;

    while (bytesRead < maxBytes && segment->readOffset < SHM_SIZE) {
        char byte = segment->start[segment->readOffset++];
        buffer[bytesRead++] = byte;
        if (byte == END_OF_READ) {
            break;
        }
    }

    buffer[bytesRead] = '\0'; // Null-terminate the buffer
    return bytesRead;
}

int main(int argc, char *argv[]) {
    char buffer[BUFFER_SIZE];
    const char *name = NULL;
    int shm_fd;
    void *ptr;
    sem_t *dataAvailable;

    if (argc == 3) {
        name = argv[1];
        dataAvailable = sem_open(argv[2], 0);
        if (dataAvailable == SEM_FAILED) {
            perror("sem_open");
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Uso: %s <nombre> <nombre del semáforo>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Abrir la memoria compartida
    shm_fd = shm_open(name, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Mapear la memoria
    ptr = mmap(0, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    shmSegment segment = { .start = (char *)ptr, .readOffset = 0, .writeOffset = 0 };

    while (1) {
        sem_wait(dataAvailable);
        size_t bytesRead = readShm(buffer, &segment, BUFFER_SIZE);
        if (bytesRead == 0) {
            break;
        }
        printf("%s", buffer); // No \n porque el buffer ya tiene el salto de línea
    }

    // Desmapear y cerrar la memoria compartida
    if (munmap(ptr, SHM_SIZE) == -1) {
        perror("munmap");
        exit(EXIT_FAILURE);
    }

    if (close(shm_fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }

    if (sem_close(dataAvailable) == -1) {
        perror("sem_close");
        exit(EXIT_FAILURE);
    }

    return 0;
}