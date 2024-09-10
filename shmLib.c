// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include "shmLib.h"

#define SEM_NAME_DATA_AVAILABLE "/dataAvailableSemaphore"
#define SEM_NAME_MUTEX "/mutex"
//#define SHM_SIZE 1024
//#define END_OF_READ '\0'

typedef struct sharedMemoryCDT {
    char *name;
    char *start;
    int fd;
    int size;
    int readOffset;
    int writeOffset;
    sem_t *dataAvailable;
    sem_t *mutex; //@TODO cambiar a tipo MUTEX?
} sharedMemoryCDT;

sharedMemoryADT getShm(const char *name, int oflag, mode_t mode) {
    sharedMemoryADT shm = malloc(sizeof(sharedMemoryCDT));
    if (shm == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    shm->name = strdup(name);
    if (shm->name == NULL) {
        perror("strdup failed");
        exit(EXIT_FAILURE);
    }

    if (oflag & O_CREAT) {
        sem_unlink(SEM_NAME_DATA_AVAILABLE);
        sem_unlink(SEM_NAME_MUTEX);
        shm_unlink(name);
    }

    shm->fd = shm_open(shm->name, oflag, mode);
    if (shm->fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    if (oflag & O_CREAT) {
        if (ftruncate(shm->fd, SHM_SIZE) == -1) {
            perror("ftruncate failed");
            exit(EXIT_FAILURE);
        }
    }

    shm->start = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm->fd, 0);
    if (shm->start == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    shm->dataAvailable = sem_open(SEM_NAME_DATA_AVAILABLE, O_CREAT, 0644, 1);
    if (shm->dataAvailable == SEM_FAILED) {
        perror("sem_open for dataAvailable failed");
        exit(EXIT_FAILURE);
    }

    shm->mutex = sem_open(SEM_NAME_MUTEX, O_CREAT, 0644, 1);
    if (shm->mutex == SEM_FAILED) {
        perror("sem_open for mutex failed");
        exit(EXIT_FAILURE);
    }

    shm->size = SHM_SIZE;
    shm->readOffset = 0;
    shm->writeOffset = 0;

    return shm;
}

size_t writeShm(const char *buffer, sharedMemoryADT segment, size_t bufferSize) {
    size_t bytesWritten = 0;

    sem_wait(segment->mutex);
    while (bytesWritten < bufferSize && segment->writeOffset < SHM_SIZE) {
        char byte = buffer[bytesWritten++];
        segment->start[segment->writeOffset++] = byte;
        if (byte == END_OF_READ) {
            break;
        }
    }
    sem_post(segment->mutex);

    sem_post(segment->dataAvailable);
    return bytesWritten;
}


size_t readShm(char *buffer, sharedMemoryADT segment, size_t bufferSize) {
    size_t bytesRead = 0;
    sem_wait(segment->dataAvailable);
    sem_wait(segment->mutex);
    while (bytesRead < bufferSize && segment->readOffset < SHM_SIZE) {
        char byte = segment->start[segment->readOffset++];
        buffer[bytesRead] = byte;
        if (byte == END_OF_READ) {
            break;
        }
        bytesRead++;
    }
    sem_post(segment->mutex);
    return bytesRead;
}


void closeShm(sharedMemoryADT segment) {
    if (sem_close(segment->mutex) == -1) {
        perror("sem_close for mutex failed");
        exit(EXIT_FAILURE);
    }
    if (sem_close(segment->dataAvailable) == -1) {
        perror("sem_close for dataAvailable failed");
        exit(EXIT_FAILURE);
    }

    if (munmap(segment->start, SHM_SIZE) == -1) {
        perror("munmap failed");
        exit(EXIT_FAILURE);
    }

    if (close(segment->fd) == -1) {
        perror("close failed");
        exit(EXIT_FAILURE);
    }

    free(segment->name);
    free(segment);
    segment=NULL;
}
