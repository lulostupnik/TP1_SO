
// El esclavo va recibiendo PATHs de archivos por la entrada estandar

// Para cada archivo crea un fork() y un execve() para ejecutar hashmd5 de ese archivo
// notar que hashmd5 imprime el resultado junto con el nombre del archivo en la salida estandar
// Si queremos que se imprima el PID del esclavo lo podemos imprimir justo antes de ejecutar hashmd5

// Si el master cierra el pipe de escritura, el read del esclavo retorna -1 creo y ahí podemos hacer que termine

// #include <stdio.h>
// #include <string.h>

// #define MAX_LINE_LENGTH 1024

// int main() {
//     char line[MAX_LINE_LENGTH];
//     while(1){

    
//     // Read from standard input until EOF
//     while (fgets(line, sizeof(line), stdin)) {
//         // Remove the newline character if present
//         size_t len = strlen(line);
//         if (len > 0 && line[len - 1] == '\n') {
//             line[len - 1] = '\0'; // Remove the newline character
//         }

//         // Print the processed line
//         fprintf(STDERR_FILENO,"Processed line: %s\n", line);
//     }
//     }
//     return 0;
// }


#include <stdio.h>
#include <string.h>
#include <unistd.h> // For write()

#define MAX_LINE_LENGTH 1024

int main() {
    char line[MAX_LINE_LENGTH];
    
    // Read from standard input until EOF or error
    while (fgets(line, sizeof(line), stdin) != NULL) {
        // Remove the newline character if present
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0'; // Remove the newline character
        }

        // Print the processed line to stderr
        // Note: write expects a buffer length, not null-terminated string
        char msg[MAX_LINE_LENGTH + 20];
        int msg_len = snprintf(msg, sizeof(msg), "%d (PID). Processed line: %s\n", getpid(), line);
        write(STDERR_FILENO, msg, msg_len);
    }

    return 0;
}
