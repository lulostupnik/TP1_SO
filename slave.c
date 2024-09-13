// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "slave.h"

#define MD5SUM_STRLEN_PLUS_NULL_TERMINATED 7
#define MAX_COMMAND_LENGTH (MAX_PATH_LENGTH + MD5SUM_STRLEN_PLUS_NULL_TERMINATED ) // 7 = 6 de "md5sum " + 1 de \0
#define MAX_PID_DIGITS 7
#define MD5_OUT_LEN 32

int main()
{
    setvbuf(stdout, NULL, _IONBF, 0);
    char *path = NULL;
    size_t len = 0;
    ssize_t bytes_read = 0;
    char command[MAX_COMMAND_LENGTH];
    FILE *fp;
    char response[MD5_OUT_LEN + MAX_PATH_LENGTH + 2]; // 2 = 1 espacio + 1 \0

    while ((bytes_read = getline(&path, &len, stdin)) != -1)
    {
        // todo -> chequear si el path es mayor a MAX_PATH_LENGTH
        // todo -> if (bytes_read > 0 && path[bytes_read - 1] == '\n')
        path[bytes_read - 1] = '\0';

        snprintf(command, MAX_COMMAND_LENGTH, "md5sum %s", path); // todo -> chequear si ahí va MAX_COMMAND_LENGTH

        fp = popen(command, "r");
        if (fp == NULL)
        {
            // fprintf(stderr, "Error popen en path; %s", path);
            perror("Error popen");
            continue;
        }
        // todo -> chequear después del fgets ver si fallo md5sum

        /* todo -> chequear si el fgets falla

         if (fgets(response, sizeof(response), fp) == NULL)
        {
            perror("Error leyendo la respuesta de md5sum");
            pclose(fp);
            continue;
        }
        */
        fgets(response, MD5_OUT_LEN + MAX_PATH_LENGTH + 3, fp); // +2 por los espacios, +1 por \n, no se tiene en cuenta el \0, puesto que fgets lo agrega

        printf("%d %s", getpid(), response);
        // todo -> chequear si está bien el parametro del medio

        // todo -> no se si se necesita manejar el error
        int status = pclose(fp);
        if (status == -1)
        {
            // todo -> si esto falla debería hacer un exit? Si es así no me tengo que olvidar de liberar el path
            perror("Error al cerrar el pipe");
        }
    }

    free(path);

    return 0;
}