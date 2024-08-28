
// El esclavo va recibiendo PATHs de archivos por la entrada estandar

// Para cada archivo crea un fork() y un execve() para ejecutar hashmd5 de ese archivo
// notar que hashmd5 imprime el resultado junto con el nombre del archivo en la salida estandar
// Si queremos que se imprima el PID del esclavo lo podemos imprimir justo antes de ejecutar hashmd5

// Si el master cierra el pipe de escritura, el read del esclavo retorna -1 creo y ahí podemos hacer que termine

/* sobre read (man)

If successful, the number of bytes actually read is returned.  Upon
reading end-of-file, zero is returned.  Otherwise, a -1 is returned and
the global variable errno is set to indicate the error.

*/
#include "slave.h"

void read_path(char *buffer)
{

    int bytes_read;

    if ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytes_read] = '\0';
    }
    else
    {
        // Cuando el master cierre su pipe de escritura read
        perror("Error");
        exit(EXIT_FAILURE);
    }
}

void print_md5(char *buffer)
{
    int pid;
    pid = fork();
    if (pid == -1)
    {
        perror("error fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0)
    {
        char *argv[] = {"/usr/local/bin/md5sum", buffer, NULL};
        if (execve("/usr/local/bin/md5sum", argv, NULL) == -1)
        {
            perror("Error ejecutando md5sum");
            exit(EXIT_FAILURE);
        };
    }
    return;
}

int main()
{
    setvbuf(stdout, NULL, _IONBF, 0); // Lo que mostro agodio en ejemplo

    char buffer[BUFFER_SIZE];

    while (1)
    {
        read_path(buffer);

        print_md5(buffer);
    }
}
