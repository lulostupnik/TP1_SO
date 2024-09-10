// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include "view.h"
#include "shmLib.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    char buffer[BUFFER_SIZE];
    sharedMemoryADT segment;
    //setvbuf(stdin, NULL, _IONBF, 0);  // AVERIGUAR
    setvbuf(stdout, NULL, _IONBF, 0); 
    if (argc == 2) {
        segment = getShm(argv[1],O_RDWR,0666);
    }else if(argc ==1){
        char * buffer_input;
        size_t buffer_size = BUFFER_SIZE;
        int bytes = getline(&buffer_input, &buffer_size, stdin);
        if(bytes == -1){
            free(buffer_input);
            perror("Get line");
            exit(EXIT_FAILURE);
        }
        buffer_input[bytes-1] = 0;
        segment = getShm(buffer_input, O_RDWR, 0666);
        free(buffer_input);

    } else {
        fprintf(stderr, "Wrong Usage\n"); //cambiar
        exit(EXIT_FAILURE);
    }
    //printf("Segment view %p\n", segment);


    while (1) {
        
        size_t bytesRead = readShm(buffer, segment, BUFFER_SIZE);

        if (bytesRead == 1) {
            break;
        }

        printf("%s", buffer); // No \n porque el buffer ya tiene el salto de línea
    }


    // Desmapear y cerrar la memoria compartida
    closeShm(segment);



    return 0;
}