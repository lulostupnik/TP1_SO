// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "md5.h"
#include "slave.h"

#define PIPE_WRITE 1
#define PIPE_READ 0
#define SHM_NAME "md5_app_shm"
#define MODE 0666

//@TODO agregar checkeo de error en close y en dup2

#define RED "\033[31m"
#define WHITE "\033[37m"

static inline void close_fd(int fd){
    if(close(fd) == -1){
        perror("close");
        exit(EXIT_FAILURE);
    }
}

static inline void close_both_fds(int pipe[2]){
    close_fd(pipe[PIPE_READ]);
    close_fd(pipe[PIPE_WRITE]);
}

static inline void pipe_(int pipefd[2]){
    if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
    }
}




int count_newline_strlen(char *str, int * len) {

    int count = 0;
    *len = 0;

    while (*str) {
        if (*str == '\n') {
            count++;
        }
        str++;
        (*len) ++;
    }

    return count;
}



//@TODO ver si usar STRLEN o no
//@TODO va el +1????????????????????
//@TODO mando de a 1 file o de a varios
//@TODO CHECKEAR lo que retorna o que 
static int send_file(int fd, char * buff){
    int len = strlen(buff);
    if(len >= MAX_FILE_PATH_LENGHT){
        return -1;
    }

    //printf(RED"%s"WHITE "\n", buff);

    buff[len] = END_OF_READ;
    printf("My eof is %d\n", END_OF_READ );

   // size_t file_lenght = strlen(file_name);
    if(write(fd, buff, len+1) == -1){
        perror("write");
        exit(EXIT_FAILURE);
    }
    
    buff[len] = 0;
    return 1;
}


//ESTA FUNCION DEBERIA SER PARA PONER EN SHARED MEMORY LO QUE ME MANDAN !
//@TODO MEJORAR ESTA FUNCION.....
static void read_aux(int fd, char * buffer){
    
    ssize_t bytes_read = read(fd, buffer, BUFFER_SIZE-1);    //@TODO ACA SE PUEDE CORTAR A LA MITAD ALGO QUE SE LEE !
    buffer[bytes_read] = 0;
    // if (bytes_read > 0) {
    //     printf("Read %zd bytes from file descriptor %d\n", bytes_read, fd);            
    //     printf("What i read: %s\n", buffer);           
    // } else if (bytes_read == 0) {
    //     printf("End of file on file descriptor %d\n", fd);
    // } else {
    if(bytes_read<0){    
        perror("read error");
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char *argv[]){
    
    if(argc <= 1){
        perror("Error: No input files specified.\nUsage: ./md5 <file1> [file2 ... fileN]\n");
        return -1;
    }
    if(setvbuf(stdout, NULL, _IONBF, 0)!= 0){
        perror("buffer printf");
        exit(EXIT_FAILURE);
    }  // AVERIGUAR

   
    sharedMemoryADT shm = getShm(SHM_NAME, O_CREAT | O_RDWR, MODE);
    printf("%s\n", SHM_NAME);  //@TODO este es el buffer de llegada? o se refiere a otra cosa. 
    sleep(5);
    unlinkShm(shm); 

    int ans_fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644); //@TODO CHECK
    if (ans_fd == -1) {
        perror("open");
        return 1;
    }
  
    FILE *file = fdopen(ans_fd, "w");
    if (file == NULL) {
        perror("fdopen");
        close(ans_fd);
        exit(EXIT_FAILURE);
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
        if(pipefd_parent_read[PIPE_READ] > highest_read_fd){
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
            for(int j=0;j<i ; j++){
                close_fd(childs_pipe_fds_read[j]);
                close_fd(childs_pipe_fds_write[j]);  //En los hijos se quedaban abiertos los FDS.  Checkear
            }

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

    
    

    for(int i=0; i<CANT_SLAVES && files_sent+1 < argc ; i++){
        for(int j=0; j<FILES_PER_SLAVE && files_sent+1 < argc ; j++ ){
            send_file(childs_pipe_fds_write[i],argv[1+files_sent++]);
        } 
    }
    

    // nfds  should  be  set  to  the  highest-numbered file descriptor in any of the three sets, plus 1.  The indicated file descriptors in each set are checked, up to this limit (but see BUGS).
    
    //Me quedan archivos para mandar

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

        int buff_len = 0;
        // Check which file descriptors are ready
        for(int i = 0; i < CANT_SLAVES && fds_ready_cant > 0; i++) {
            if (FD_ISSET(childs_pipe_fds_read[i], &readfds)) {
                read_aux(childs_pipe_fds_read[i], string_from_fd);   //@TODO ACA VA LO DE SHARED MEMORY. (no sacar que se ponga el file read en el buffer xq se me rompe todo )
                //files_read += write_shm(string_from_fd, shm);
                files_read += count_newline_strlen(string_from_fd, &buff_len);
                
                writeShm(string_from_fd, shm, buff_len);
                fprintf(file, "%s", string_from_fd); //esto cambiarlo al final. tardaria menos. 

                //printf("BUFFER\n:%s", string_from_fd);
                if(files_sent < argc -1){
                    send_file(childs_pipe_fds_write[i], argv[1+files_sent++]);
                }
                fds_ready_cant--;
            }
        }
    }
    char EOF_BUFF[1] = {END_OF_READ};
    writeShm(EOF_BUFF, shm, 1);    
    fflush(file);
    fclose(file);
    close(ans_fd);
    closeShm(shm);


    for(int i=0; i<CANT_SLAVES; i++){
        close_fd(childs_pipe_fds_read[i]);
        close_fd(childs_pipe_fds_write[i]);
    }
    int childs_left = CANT_SLAVES;
    while(childs_left){
        if(wait(NULL) == -1){
            perror("Wait");
        }
        childs_left--;
    }

   //while(1);
   return 0;
}


