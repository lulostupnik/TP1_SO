
// Este esclavo soporta que el path completo no se mande en un solo write
// sino que se mande en varios write
// Para esto se usa un buffer que se va llenando con los distintos writes
// y cuando se lee un \n se ejecuta el hashmd5
//
#include "slave.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

//@todo manejar el caso de cierre de pipe de lectura

void print_md5(char *buffer);

// función que dado char *
// encuentra el \n, guarda el tamaño de la línea, reemplaza el \n por \0
// @todo reemplazar por strchr o alguna función de la librería estándar
int find_newline(char *buffer)
{
    int i;
    for (i = 0; buffer[i] != '\n' && buffer[i] != '\0'; i++)
    {
    }

    if (buffer[i] == '\0')
    { /* @todo manejar caso/error */
    }

    buffer[i] = '\0';
    return i;
}

int read_path(char *buffer)
{

    int bytes_read;
    if ((bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1)) > 0)
    {
        return bytes_read;
    }
    else
    {
        perror("Pipe de lectura cerrado");
        exit(EXIT_FAILURE);
    }
}

void chequear_path(char *path)
{
    // chequear que el path sea válido
    // @todo completar
    return;
}

void run_md5sum(char *path)
{
    // @todo cambiar BUFFER_SIZE
    // pipefd[0] es para leer
    // pipefd[1] es para escribir
    // Buffer para leer el resultado de md5sum

    /*
    char buffer[BUFFER_SIZE];
    int pipefd[2];
    */

    /*
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    */

    int pid;
    pid = fork();
    if (pid == -1)
    {
        perror("error fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        /*
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        */
        printf("%d ", getpid()); // @todo cambiar para que se imprima todo de una (usando pipe)
        char *argv[] = {"/usr/local/bin/md5sum", path, NULL};

        if (execve("/usr/local/bin/md5sum", argv, NULL) == -1)
        {
            perror("Error ejecutando md5sum");
            exit(EXIT_FAILURE);
        };
    }

    // close(pipefd[1]);

    // snprintf(buffer, BUFFER_SIZE, "%d %s", getpid(), path);
    waitpid(pid, NULL, 0); // Espera a que el hijo termine
    // printf("Running md5sum on %s\n", path);
}

void consume_path(char *buffer, int *idxstart, int *idxend)
{
    int cmdlength = 0;
    if (*idxend == 0) // Si el buffer está vacío leemos de stdin
    {
        *idxend = read_path(buffer);
    }

    cmdlength = find_newline(buffer + *idxstart);

    // chequear_path(buffer + *idxstart);

    run_md5sum(buffer + *idxstart);

    *idxstart += cmdlength + 1; // +1 para saltear el \n

    if (*idxstart >= *idxend) // Si ya leímos todo el buffer
    {
        *idxstart = 0;
        *idxend = 0;
    }
}

int main(int argc, char const *argv[])
{
    char buffer[BUFFER_SIZE];
    int idxstart = 0;
    int idxend = 0;

    while (1)
    {
        consume_path(buffer, &idxstart, &idxend);
    }
    // nunca llegamos acá
    return 0;
}

// BUFFER = ""
// read()
// BUFFER = "marce\nbigo\ntri.txt\nslave.c\nbig\nview.c\n"
//
