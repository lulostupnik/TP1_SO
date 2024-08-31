#include "md5.h"
#include "slave.h"

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

int count_newlines(const char *str) {
    int count = 0;
    while (*str) {
        if (*str == '\n') {
            count++;
        }
        str++;
    }
    return count;
}



//@TODO ver si usar STRLEN o no
//@TODO va el +1????????????????????
//@TODO mando de a 1 file o de a varios
static int send_file(int fd, char * buff){
    int len = strlen(buff);
    if(len >= MAX_FILE_PATH_LENGHT){
        return -1;
    }

    //printf(RED"%s"WHITE "\n", buff);

    buff[len] = '\n';

   // size_t file_lenght = strlen(file_name);
    if(write(fd, buff, len+1) == -1){
        perror("write");
        exit(EXIT_FAILURE);
    }
    
    buff[len] = 0;

}


//ESTA FUNCION DEBERIA SER PARA PONER EN SHARED MEMORY LO QUE ME MANDAN !
//@TODO MEJORAR ESTA FUNCION.....
static void read_aux(int fd, char * buffer){
    
    size_t bytes_read = read(fd, buffer, BUFFER_SIZE-1);
    buffer[bytes_read] = 0;
    if (bytes_read > 0) {
        printf("Read %zd bytes from file descriptor %d\n", bytes_read, fd);            
        printf("What i read: %s\n", buffer);           
    } else if (bytes_read == 0) {
        printf("End of file on file descriptor %d\n", fd);
    } else {
        perror("read error");
    }
}


int main(int argc, char *argv[]){
    
    if(argc <= 1){
        perror("Error: No input files specified.\nUsage: ./md5 <file1> [file2 ... fileN]\n");
        return -1;
    }

    pid_t pid;
    const char *pathname = "./slave";
    char *const argv_[] = { "./slave", NULL };  
    char *const envp_[] ={NULL};

    int childs_pipe_fds_write[CANT_SLAVES] = {}; // aca el master escribe
    int childs_pipe_fds_read[CANT_SLAVES] = {};
    int highest_read_fd = 0;

    int pipefd_parent_write[2];
    int pipefd_parent_read[2];


    fd_set readfds;
    //FD_ZERO(&readfds);

    for(int i=0; i<CANT_SLAVES ; i++){
        
        pipe_(pipefd_parent_write);
        childs_pipe_fds_write[i] = pipefd_parent_write[PIPE_WRITE]; 



        pipe_(pipefd_parent_read); 
        int read_fd = pipefd_parent_read[PIPE_READ];
        childs_pipe_fds_read[i] = read_fd;
        //Things for select function
        if(pipefd_parent_read[PIPE_READ]){
            highest_read_fd = read_fd;
        }
        //FD_SET(read_fd, &readfds);
        // ...... //



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


    //OBS: con esta implementacion mando FILES_PER_SLAVE sin importar si quedan slaves sin files si FILES es mas chico que FILES_PER_SLAVE * CANT_SLAVES 
    //@TODO fijarse si va el files sent en ambos fors
    int files_sent = 0;
    int files_in_buffer = 0;
    
    

    for(int i=0; i<CANT_SLAVES && files_sent+1 < argc ; i++){
        for(int j=0; j<FILES_PER_SLAVE && files_sent+1 < argc ; j++ ){
            send_file(childs_pipe_fds_write[i],argv[1+files_sent++]);
        } 
    }
    

    // nfds  should  be  set  to  the  highest-numbered file descriptor in any of the three sets, plus 1.  The indicated file descriptors in each set are checked, up to this limit (but see BUGS).
    
    //Me quedan archivos para mandar
    int fds_ready_cant = 0;
    int files_read = 0;
    char string_from_fd[BUFFER_SIZE];
    //@TODO checkear si siempre hay que hacer el FD_SET si o SI. 
    while (files_read < argc - 1) {
        FD_ZERO(&readfds);
        for(int i = 0; i < CANT_SLAVES; i++) {
            FD_SET(childs_pipe_fds_read[i], &readfds);
        }

        // Call select
        int fds_ready_cant = select(highest_read_fd + 1, &readfds, NULL, NULL, NULL);
        if (fds_ready_cant == -1) {
            perror("select error");
            exit(EXIT_FAILURE);
        }

      
        // Check which file descriptors are ready
        for(int i = 0; i < CANT_SLAVES && fds_ready_cant > 0; i++) {
            if (FD_ISSET(childs_pipe_fds_read[i], &readfds)) {
                read_aux(childs_pipe_fds_read[i], string_from_fd);   //@TODO ACA VA LO DE SHARED MEMORY. (no sacar que se ponga el file read en el buffer xq se me rompe todo )
                files_read += count_newlines(string_from_fd);
                if(files_sent < argc -1){
                    send_file(childs_pipe_fds_write[i], argv[1+files_sent++]);
                }
                fds_ready_cant--;
            }
        }
}
 

}




/*
master(){
    while(1){
        publicar_resultado_en_shm(...); // agreg



    }
}*/
