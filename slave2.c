
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
       // buffer[bytes_read] = '\0';

        //fprintf(stderr, "Leí lo siguiente: %s\n", buffer);

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
    fprintf(stderr, "(%d) Running md5sum on %s\n", getpid(), path);
    //printf("Running md5sum on %s\n", path);
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
