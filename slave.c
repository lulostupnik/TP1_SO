// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "slave.h"

int main()
{
    if(setvbuf(stdout, NULL, _IONBF, 0) != 0){
        perror("Error: Failed to disable buffering for stdout");
        exit(EXIT_FAILURE);
    }

    char *path = NULL;
    size_t len = 0;
    ssize_t bytes_read = 0;
    FILE *fp;
    char command[MAX_COMMAND_LENGTH];
    char response[MAX_MD5_OUT_LEN];

    while ((bytes_read = getline(&path, &len, stdin)) != -1)
    {
        if(bytes_read > MAX_PATH_LENGTH){
            free(path);
            fprintf(stderr, "Error: Path len is greater than maximum (%d) in slave %d", MAX_PATH_LENGTH, getpid());
            exit(EXIT_FAILURE);
        }
        path[bytes_read - 1] = '\0';

        snprintf(command, MAX_COMMAND_LENGTH, "md5sum %s", path);

        fp = popen(command, "r");
        if (fp == NULL){
            free(path);
            fprintf(stderr, "Error: Failed to execute popen() for command '%s'\n", command);
            exit(EXIT_FAILURE);
        }
       
        fgets(response,MAX_MD5_OUT_LEN, fp);
        printf("%d %s", getpid(), response);
       
        int status = pclose(fp);
        if (status == -1){
            perror("Error: Failed to close the path");
        }
    }

    free(path);
    return 0;
}