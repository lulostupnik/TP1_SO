#include "md5.h"

#define PIPE_WRITE 1
#define PIPE_READ 0

//@TODO agregar checkeo de error en close y en dup2

#define RED "\033[31m"
#define WHITE "\033[37m"

static void close_fd(int fd){
    if(close(fd) == -1){
        perror("close");
        exit(EXIT_FAILURE);
    }
}

static void close_both_fds(int pipe[2]){
    close_fd(pipe[PIPE_READ]);
    close_fd(pipe[PIPE_WRITE]);
}

static void pipe_(int pipefd[2]){
    if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
    }
}


//@TODO ver si usar STRLEN o no
//@TODO va el +1????????????????????
//@TODO mando de a 1 file o de a varios
static int send_file(int fd, char * buff){
    int len = strlen(buff);
    if(len >= MAX_FILE_PATH_LENGHT){
        return -1;
    }

    printf(RED"%s"WHITE "\n", buff);

    buff[len] = '\n';

   // size_t file_lenght = strlen(file_name);
    if(write(fd, buff, len+1) == -1){
        perror("write");
        exit(EXIT_FAILURE);
    }
    
    buff[len] = 0;

}





int main(int argc, char *argv[]){
    
    if(argc <= 1){
        perror("Error: No input files specified.\nUsage: ./md5 <file1> [file2 ... fileN]\n");
        return -1;
    }

    pid_t pid;
    const char *pathname = "./slave2";
    char *const argv_[] = { "./slave2", NULL };  
    char *const envp_[] ={NULL};

    int childs_pipe_fds_write[CANT_SLAVES] = {};
    int childs_pipe_fds_read[CANT_SLAVES] = {};
    int pipefd_parent_write[2];
    int pipefd_parent_read[2];

    for(int i=0; i<CANT_SLAVES ; i++){
        
        pipe_(pipefd_parent_write);
        childs_pipe_fds_write[i] = pipefd_parent_write[PIPE_WRITE]; 

        pipe_(pipefd_parent_read); 
        childs_pipe_fds_read[i] = pipefd_parent_read[PIPE_READ];

        pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {  
            //Como hijo pongo mi STDOUT como el pipe en donde lee el padre y cierro ambos FDS
            if(dup2(pipefd_parent_read[PIPE_WRITE], STDOUT_FILENO) == -1){
                perror("dup");
                exit(EXIT_FAILURE);
            }
            close_both_fds(pipefd_parent_read);

            //Como hijo pongo mi STDIN en donde escribe el padre y cierro ambos FDS
            if(dup2(pipefd_parent_write[PIPE_READ], STDIN_FILENO)){
                perror("dup");
                exit(EXIT_FAILURE);
            }
            close_both_fds(pipefd_parent_write);

            execve(pathname, argv_, envp_);
            perror("execve");
            exit(EXIT_FAILURE);
        }
        //Soy el padre:
        close_fd(pipefd_parent_read[PIPE_WRITE]);
        close_fd(pipefd_parent_write[PIPE_READ]);
    }

    //Ya tengo todos los SLAVES creados



    /*
    *
    *
    *  A PARTIR DE AHORA PUEDE TENER ERRORES : 
    *
    */

    //OBS: con esta implementacion mando FILES_PER_SLAVE sin importar si quedan slaves sin files si FILES es mas chico que FILES_PER_SLAVE * CANT_SLAVES 
    //@TODO fijarse si va el files sent en ambos fors
    int files_sent = 0;
    int files_in_buffer = 0;
    for(int i=0; i<CANT_SLAVES && files_sent+1 < argc ; i++){
        for(int j=0; j<FILES_PER_SLAVE && files_sent+1 < argc ; j++ ){
            send_file(childs_pipe_fds_write[i],argv[1+files_sent++]);
        } 
    }

 



    //int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
    // int fd_slave = 1;
    // fd_set slaves_fd_set;
    // FD_SET(fd_slave, &slaves_fd_set);
    // int higher_fd_plus_1 = 1;
    // int fds_ready = select(higher_fd_plus_1, &slaves_fd_set, NULL, NULL, NULL);
}




/*
master(){
    while(1){
        publicar_resultado_en_shm(...); // agreg



    }
}

// Use select to monitor the file descriptors
    fd_set readfds;
    int max_fd = 0;

    // Determine the highest file descriptor
    for (int i = 0; i < CANT_SLAVES; i++) {
        if (childs_pipe_fds_read[i] > max_fd) {
            max_fd = childs_pipe_fds_read[i];
        }
    }

    // Monitoring the child pipes using select
    while (1) {
        FD_ZERO(&readfds);  // Initialize the file descriptor set

        // Add the child's read ends to the readfds set
        for (int i = 0; i < CANT_SLAVES; i++) {
            FD_SET(childs_pipe_fds_read[i], &readfds);
        }

        // Wait for any of the file descriptors to be ready for reading
        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // Check which file descriptors are ready
        for (int i = 0; i < CANT_SLAVES; i++) {
            if (FD_ISSET(childs_pipe_fds_read[i], &readfds)) {
                char buffer[1024];
                int bytes_read = read(childs_pipe_fds_read[i], buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';  // Null-terminate the string
                    printf("Received from slave %d: %s", i, buffer);
                } else if (bytes_read == 0) {
                    // EOF: child has closed the pipe
                    close_fd(childs_pipe_fds_read[i]);
                } else {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    return 0;
}


*/

/*

*/