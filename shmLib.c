#include "shmLib.h"

typedef struct sharedMemoryCDT {
    char *name;
    char *start;
    int fd;
    int size;
    int readOffset;
    int writeOffset;
} sharedMemoryCDT;

sharedMemoryADT getShm(const char *name, int oflag, mode_t mode) {
    sharedMemoryADT shm = malloc(sizeof(sharedMemoryCDT));
    if (shm == NULL) {
        perror("malloc failed :(");
        exit(EXIT_FAILURE);
    }

    shm->name = strdup(name);
    if (shm->name == NULL) {
        perror("strdup failed :(");
        exit(EXIT_FAILURE);
    }

    shm->fd = shm_open(name, oflag, mode);
    if (shm->fd == -1) {
        perror("shm_open failed :(");
        exit(EXIT_FAILURE);
    }

    if (oflag & O_CREAT) {
        if (ftruncate(shm->fd, SHM_SIZE) == -1) {
            perror("ftruncate failed :(");
            exit(EXIT_FAILURE);
        }
    }
    if(oflag & O_RDWR){
        shm->start = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm->fd, 0);
    } else {
        shm->start = mmap(0, SHM_SIZE, PROT_READ, MAP_SHARED, shm->fd, 0);
    }
    if (shm->start == MAP_FAILED) {
        perror("mmap failed :(");
        exit(EXIT_FAILURE);
    }

    shm->size = SHM_SIZE;
    shm->readOffset = 0;
    shm->writeOffset = 0;

    return shm;
}

size_t writeShm(const char *buffer, sharedMemoryADT segment, size_t bufferSize) {
    size_t bytesWritten = 0;

    while (bytesWritten < bufferSize && segment->writeOffset < SHM_SIZE) {
        char byte = buffer[bytesWritten++];
        segment->start[segment->writeOffset++] = byte;
        if (byte == END_OF_READ) {
            break;
        }
    }


    return bytesWritten;
}

size_t readShm(char *buffer, sharedMemoryADT segment, size_t bufferSize) {
    size_t bytesRead = 0;

    while (bytesRead < bufferSize && segment->readOffset < SHM_SIZE) {
        char byte = segment->start[segment->readOffset++];
        buffer[bytesRead++] = byte;
        if (byte == END_OF_READ) {
            break;
        }
    }

    if(bytesRead > bufferSize){
        buffer[bytesRead] = '\0'; 
    }

    return bytesRead;
}

void closeShm(sharedMemoryADT segment) {
    if (munmap((segment)->start, SHM_SIZE) == -1) {
        perror("munmap failed :(");
        exit(EXIT_FAILURE);
    }

    if (close((segment)->fd) == -1) {
        perror("close failed :(v");
        exit(EXIT_FAILURE);
    }

    free((segment)->name);
    free(segment);
    segment = NULL;
}