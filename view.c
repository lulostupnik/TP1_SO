// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include "view.h"
#include "shm_lib.h"
#include "slave.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    char buffer[BUFFER_SIZE_VIEW];
    shared_memory_adt segment;

    if(setvbuf(stdout, NULL, _IONBF, 0)!= 0){
        perror("Error: Failed to disable buffering for stdout");
        return 1;
    }  

    if (argc == 2) {
        segment = get_shm(argv[1],false,false);
    }else if(argc ==1){
        setvbuf(stdin, NULL, _IONBF, 0);  
        if(setvbuf(stdin, NULL, _IONBF, 0)!= 0){
             perror("Error: Failed to disable buffering for stdin");
        return 1;
        }  
        char * buffer_input;
        size_t buffer_size = BUFFER_SIZE_VIEW;
        int bytes = getline(&buffer_input, &buffer_size, stdin);
        if(bytes == -1){
            free(buffer_input);
            perror("Could not read line from stdin");
            return 1;
        }
        buffer_input[bytes-1] = 0;
        segment = get_shm(buffer_input, false, false);
        free(buffer_input);

    } else {
        fprintf(stderr, "Error: invalid usage\nUsage: ./md5 <file1> [file2 ... file_n] | ./view\nAlternatively, run ./md5 and provide the shared memory name through stdin.\n"); 
        return 1;
    }
    if(segment == NULL){
        return 1;
    }

    while (1) {
      
        ssize_t bytes_read = read_shm(buffer, segment, BUFFER_SIZE_VIEW);
        if ( bytes_read == 1 || bytes_read == -1) {
            break;  
           
        }
        printf("%s", buffer);
    }
    
    close_shm(segment);
    return 0;
}