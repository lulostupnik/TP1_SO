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
    sem_t *dataAvailable;
    sharedMemoryADT segment;

    if (argc == 3) {
        segment = getShm(argv[1],O_RDWR,0666);
        dataAvailable = sem_open(argv[2], 0);
        if (dataAvailable == SEM_FAILED) {
            perror("sem_open");
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Not enough arguments >:(, we only got %s out of 3\n",argc);
        exit(EXIT_FAILURE);
    }

    while (1) {
        sem_wait(dataAvailable);
        size_t bytesRead = readShm(buffer, segment, BUFFER_SIZE);
        if (bytesRead == 0) {
            break;
        }
        printf("%s", buffer); // No \n porque el buffer ya tiene el salto de línea
    }

    // Desmapear y cerrar la memoria compartida
    closeShm(segment);

    if (sem_close(dataAvailable) == -1) {
        perror("sem_close");
        exit(EXIT_FAILURE);
    }

    return 0;
}