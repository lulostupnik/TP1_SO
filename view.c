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
    setvbuf(stdout, NULL, _IONBF, 0);  // AVERIGUAR

    if (argc == 2) {
        segment = getShm(argv[1],O_RDWR,0666);
    }else if(argc ==1){
        char buffer[256];
        fgets(buffer, sizeof(buffer), stdin);
        segment = getShm(buffer, O_RDWR, 0666);
    } else {
        fprintf(stderr, "Wrong Usage\n"); //cambiar
        exit(EXIT_FAILURE);
    }

    while (1) {
        size_t bytesRead = readShm(buffer, segment, BUFFER_SIZE);
        if (bytesRead == 0) {
            break;
        }
        printf("%s", buffer); // No \n porque el buffer ya tiene el salto de línea
    }

    // Desmapear y cerrar la memoria compartida
    closeShm(segment);



    return 0;
}