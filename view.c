// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include "view.h"
#include "shm_lib.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    char buffer[BUFFER_SIZE];
    shared_memory_adt segment;
    setvbuf(stdout, NULL, _IONBF, 0);

    if (argc == 2) {
        segment = get_shm(argv[1],false,false);
    }else if(argc ==1){
        setvbuf(stdin, NULL, _IONBF, 0);  
        char * buffer_input;
        size_t buffer_size = BUFFER_SIZE;
        int bytes = getline(&buffer_input, &buffer_size, stdin);
        if(bytes == -1){
            free(buffer_input);
            perror("Get line");
            exit(EXIT_FAILURE);
        }
        buffer_input[bytes-1] = 0;
        segment = get_shm(buffer_input, false, false);
        free(buffer_input);

    } else {
        fprintf(stderr, "Wrong Usage\n"); //cambiar
        exit(EXIT_FAILURE);
    }

    while (1) {
        
        ssize_t bytes_read = read_shm(buffer, segment, BUFFER_SIZE);
        
        if ( bytes_read == 1 || bytes_read == -1) {
            break;  
           
        }
        
        printf("%s", buffer); // No \n porque el buffer ya tiene el salto de línea
    }
    
    close_shm(segment);
    return 0;
}