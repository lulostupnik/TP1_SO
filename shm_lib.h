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
#include <stdbool.h>


#define SHM_SIZE 4096
#define END_OF_READ '\0'

#define LAST_USER 1

typedef struct shared_memory_cdt * shared_memory_adt;

shared_memory_adt get_shm(const char *name, bool is_creator, bool is_writer);
ssize_t read_shm(char *buffer, shared_memory_adt segment, size_t max_bytes);
ssize_t write_shm(const char *buffer, shared_memory_adt segment, size_t buffer_size);
void close_shm(shared_memory_adt segment);
void unlink_shm(shared_memory_adt segment);
int write_to_fd_shm(shared_memory_adt segment, int fd);



#endif