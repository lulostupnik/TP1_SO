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
#define MAX_ENTRIES 10

int main() {
    const char *shm_name = "my_shared_memory";
    const char *sem_name = "/sem_example";
    int shm_fd;
    void *ptr;
    sem_t *semaforo;

    // Crear la memoria compartida
    shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Configurar el tamaño de la memoria compartida
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    // Mapear la memoria
    ptr = mmap(0, SHM_SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Crear el semáforo
    semaforo = sem_open(sem_name, O_CREAT, 0644, 0);
    if (semaforo == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Escribir múltiples entradas en la memoria compartida
    char *messages[MAX_ENTRIES] = {
        "Archivo1.txt 9e107d9d372bb6826bd81d3542a419d6 1234\n",
        "Archivo2.txt e4d909c290d0fb1ca068ffaddf22cbd0 5678\n",
        "Archivo3.txt 098f6bcd4621d373cade4e832627b4f6 9101\n",
        "Archivo4.txt ad0234829205b9033196ba818f7a872b 1121\n",
        "Archivo5.txt 8ad8757baa8564dc136c1e07507f4a98 3141\n",
        "Archivo6.txt 3d793237731e2a5bf53f40f6b89b1e3e 5161\n",
        "Archivo7.txt 73feffa4b7f6bb68e44cf984c85f6e88 7181\n",
        "Archivo8.txt e99a18c428cb38d5f260853678922e03 9202\n",
        "Archivo9.txt 7c6a180b36896a0a8c02787eeafb0e4c 1223\n",
        "Archivo10.txt d3b07384d113edec49eaa6238ad5ff00 4252\n"
    };

    char *current_ptr = (char *)ptr;
    for (int i = 0; i < MAX_ENTRIES; i++) {
        sprintf(current_ptr, "%s", messages[i]);
        current_ptr += strlen(messages[i]);
        sem_post(semaforo);  // Signal the semaphore after writing each message
    }

    // Desmapear y cerrar
    if (munmap(ptr, SHM_SIZE) == -1) {
        perror("munmap");
        exit(EXIT_FAILURE);
    }

    if (close(shm_fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }

    // Cerrar el semáforo
    if (sem_close(semaforo) == -1) {
        perror("sem_close");
        exit(EXIT_FAILURE);
    }

    return 0;
}