#ifndef VISTA_H
#define VISTA_H

#include <stdint.h>

#define SHM_SIZE 4096
#define BUFFER_SIZE 256
#define MD5_LENGTH 32
#define END_OF_READ '\n'

typedef struct shmSegment {
    char *start;
    int readOffset;
    int writeOffset;
} shmSegment;

#endif