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
#define ERROR 1


typedef struct sharedMemoryCDT {
    char * name;
    char * start; 
    int fd;
    size_t size;
    size_t readOffset; 
    size_t writeOffset; 
    sem_t *dataAvailable;
    sem_t *mutex; 
} sharedMemoryCDT;

sharedMemoryADT getShm(const char *name, int oflag, mode_t mode) {
    sharedMemoryADT shm = malloc(sizeof(sharedMemoryCDT));
    if (shm == NULL) {
        perror("Error: Memory allocation failed for sharedMemoryCDT");
        return NULL;
    }

    shm->name = strdup(name);
    if (shm->name == NULL) {
        perror("Error: Failed to duplicate the name string for shared memory");
        free(shm);
        return NULL;
    }

    //Puede retornar error pero es por si hay semaforos/shm los cuales no queremos abiertos.
    if (oflag & O_CREAT) {
        unlinkShm(shm);
    }
    
    shm->fd = shm_open(shm->name, oflag, mode);
    if (shm->fd == -1) {
        perror("Error: Failed to open shared memory");
        free(shm);
        return NULL;
    }


    if (oflag & O_CREAT) {
        if (ftruncate(shm->fd, SHM_SIZE) == -1) {
            perror("Error: Failed to set shared memory size");
            shm_unlink(shm->name);
            close(shm->fd);
            free(shm->name);
            free(shm);
            return NULL;
        }
    }

    shm->start = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm->fd, 0);
     if (shm->start == MAP_FAILED) {
        perror("Error: Memory mapping failed");
        shm_unlink(shm->name);
        close(shm->fd);
        free(shm->name);
        free(shm);
        return NULL;
    }

    shm->dataAvailable = sem_open(SEM_NAME_DATA_AVAILABLE, O_CREAT, 0644, 0);
   if  (shm->dataAvailable == SEM_FAILED) {
        perror("Error: Failed to open data availability semaphore");
        shm_unlink(shm->name);
        munmap(shm->start, SHM_SIZE);
        close(shm->fd);
        free(shm->name);
        free(shm);
        return NULL;
    }

    shm->mutex = sem_open(SEM_NAME_MUTEX, O_CREAT, 0644, 1);
    if (shm->mutex == SEM_FAILED) {
        perror("Error: Failed to open mutex semaphore");
        sem_close(shm->dataAvailable);
         shm_unlink(shm->name);
        sem_unlink(SEM_NAME_DATA_AVAILABLE); 
        munmap(shm->start, SHM_SIZE);
        close(shm->fd);
        free(shm->name);
        free(shm);
        return NULL;
    }

    shm->size = SHM_SIZE;
    shm->readOffset = 0;
    shm->writeOffset = 0;


    return shm;
}




size_t writeShm(const char *buffer, sharedMemoryADT segment, size_t bufferSize) {
    size_t bytesWritten = 0;
    sem_wait(segment->mutex);

    while (bytesWritten <= bufferSize && segment->writeOffset < SHM_SIZE) {
        char byte = buffer[bytesWritten++];
        
        segment->start[segment->writeOffset++] = byte;
        if (byte == END_OF_READ) {
            break;
        }
    }
    if(!(segment->writeOffset < SHM_SIZE)){
        perror("No more memory in shared memory");
        exit(1);
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
            buffer[bytesRead++] = 0;
            break;
        }
        bytesRead++;
    }

    if(!(segment->readOffset < SHM_SIZE)){
        perror("No more memory in shared memory");
        exit(1);
    }
    sem_post(segment->mutex);
    return bytesRead;
}



void unlinkShm(sharedMemoryADT segment){
    //DEBERIA HABER UN FLAG EN LA SHARED MEMORY PORQUE SOLO HAY QUE UNLINKEAR UNA VEZ
// Una de las dos veces da error !
    sem_unlink(SEM_NAME_DATA_AVAILABLE); //@todo NO UNLINKEAR DOS VECES ERRO CONCEPTUAL.
    sem_unlink(SEM_NAME_MUTEX);
    shm_unlink(segment->name);
}


void closeShm(sharedMemoryADT segment) {
    if (sem_close(segment->mutex) == -1) {
        perror("Error: Failed to close mutex semaphore");
    }
    if (sem_close(segment->dataAvailable) == -1) {
        perror("Error: Failed to close dataAvailable semaphore");
    }

    if (munmap(segment->start, SHM_SIZE) == -1) {
        perror("Error: Failed to unmap shared memory region");
    }

    if (close(segment->fd) == -1) {
        perror("Error: Failed to close shared memory file descriptor");
    }

    free(segment->name);
    free(segment);
    segment = NULL;
}