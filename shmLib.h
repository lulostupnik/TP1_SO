#ifndef _SHMLIB_
#define _SHMLIB_

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>


#define SHM_SIZE 4096
#define END_OF_READ '\n'

#define LAST_USER 1

typedef struct sharedMemoryCDT * sharedMemoryADT;

sharedMemoryADT getShm(const char *name, int oflag, mode_t mode);
size_t readShm(char *buffer, sharedMemoryADT segment, size_t maxBytes);
size_t writeShm(const char *buffer, sharedMemoryADT segment, size_t bufferSize);
void closeShm(sharedMemoryADT segment);

//Reads until END OF READ, or bufferSize or segment Size.  
void unlinkShm(sharedMemoryADT segment);

int writeToFdShm(sharedMemoryADT segment, int fd);

#endif