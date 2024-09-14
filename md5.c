// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "md5.h"
#include "slave.h"

#define PIPE_WRITE 1
#define PIPE_READ 0
#define SHM_NAME "md5_app_shm"
#define MODE 0666
#define MIN(a,b) (((a) < (b)) ? (a):(b))
#define MAX(a,b) (((a) > (b)) ? (a):(b))

#define ERROR 1

//@TODO agregar checkeo de error en close y en dup2

static inline void close_fd(int fd){
    if(close(fd) == -1){
        fprintf(stderr, "Error: could not close fd %d\n", fd);
    }
}

static inline void close_both_fds(int pipe[2]){
    close_fd(pipe[PIPE_READ]);
    close_fd(pipe[PIPE_WRITE]);
}

// static inline void pipe_(int pipefd[2]){
//     if (pipe(pipefd) == -1) {
//             perror("pipe");
//             exit(EXIT_FAILURE);
//     }
// }


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



static int send_file(int fd, char * buff){
    int len = strlen(buff);
    if(len >= MAX_PATH_LENGTH){
        return -1;
    }
    buff[len] = '\n';
    
    if(write(fd, buff, len+1) == -1){
        perror("write");
        exit(EXIT_FAILURE);
    }
    
    buff[len] = 0;
    return 0;
}


static void read_aux(int fd, char * buffer){
    
    ssize_t bytes_read = read(fd, buffer, BUFFER_SIZE-1);    //@TODO ACA SE PUEDE CORTAR A LA MITAD ALGO QUE SE LEE !
    buffer[bytes_read] = 0;

    if(bytes_read<0){    
        perror("read error");
        exit(EXIT_FAILURE);
    }
}

static int set_up_slave(int slave_num, int childs_pipe_fds_read[], int childs_pipe_fds_write[], int pipefd_parent_read[], int pipefd_parent_write[] ){
            int flag = 0;
            for(int j=0;j<slave_num ; j++){
                close_fd(childs_pipe_fds_read[j]);
                close_fd(childs_pipe_fds_write[j]);  //En los hijos se quedaban abiertos los FDS.  Checkear
            }

            //Como hijo pongo mi STDOUT como el pipe en donde lee el padre y cierro ambos FDS
            if(dup2(pipefd_parent_read[PIPE_WRITE], STDOUT_FILENO) == -1){
                perror("Error: could not change slave stdout\n");
                flag = ERROR;
            }
            close_both_fds(pipefd_parent_read);

            //Como hijo pongo mi STDIN en donde escribe el padre y cierro ambos FDS
            if(dup2(pipefd_parent_write[PIPE_READ], STDIN_FILENO)){
                perror("Error: could not change slave stdin\n");
                flag = ERROR;
            }
            close_both_fds(pipefd_parent_write);
            return flag;
}



static void select_function_setup(fd_set * readfds, int slaves_needed, int childs_pipe_fds_read[]){
    FD_ZERO(readfds);
    for(int i = 0; i < slaves_needed; i++) {
        FD_SET(childs_pipe_fds_read[i], readfds);
    }
}


static void resend_files_to_slave(int * files_sent, int cant_to_send, int childs_pipe_fds_write[], int slave_index, int argc, char * argv[]){
    while((*files_sent < argc -1) && cant_to_send > 0 ){
            if(send_file(childs_pipe_fds_write[slave_index], argv[1+(*files_sent)++]) != 0){
                perror("File path is longer than max");
                exit(EXIT_FAILURE);
            }
            
        cant_to_send--;
    }
}

//Todo checkear
static void clean_resources_pipe(FILE * file, int ans_fd, sharedMemoryADT shm, int slaves_to_close_fd_read, int slaves_to_close_fd_write, int childs_pipe_fds_read[], int childs_pipe_fds_write[] ){
    fclose(file);
    close(ans_fd);
    closeShm(shm);

    int max = MAX(slaves_to_close_fd_read, slaves_to_close_fd_write);

    for(int i=0; i<max; i++){
        if(slaves_to_close_fd_read-- > 0){
            close_fd(childs_pipe_fds_read[i]);
        }
        if(slaves_to_close_fd_write-- > 0){
            close_fd(childs_pipe_fds_write[i]);

        }
    }
}


static void cleanResources(FILE * file, int ans_fd, sharedMemoryADT shm, int slaves_to_close_fd, int childs_to_wait, int childs_pipe_fds_read[], int childs_pipe_fds_write[] ){
    fclose(file);
    close(ans_fd);
    closeShm(shm);
    for(int i=0; i<slaves_to_close_fd; i++){
        close_fd(childs_pipe_fds_read[i]);
        close_fd(childs_pipe_fds_write[i]);
    }
    while(childs_to_wait > 0){
        if(wait(NULL) == -1){
            perror("Wait");
        }
        childs_to_wait--;
    }
}

int main(int argc, char *argv[]){
    if(argc <= 1){
        perror("Error: No input files specified.\nUsage: ./md5 <file1> [file2 ... fileN]\n");
        return ERROR;
    }
    if(setvbuf(stdout, NULL, _IONBF, 0)!= 0){
        perror("Error: Failed to disable buffering for stdout");
        return ERROR;
    }  

    sharedMemoryADT shm = getShm(SHM_NAME, O_CREAT | O_RDWR, MODE);
    if(shm == NULL){
        perror("Error: Could not create shared memory");
        return ERROR;
    }
    printf("%s\n", SHM_NAME);  
    sleep(2);
    unlinkShm(shm); 

    int ans_fd = open("resultado.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644); //@TODO CHECK
    if (ans_fd == -1) {
        closeShm(shm);
        perror("Error: Could not open file to store md5 answers.");
        return ERROR;
    }
  
    FILE *file = fdopen(ans_fd, "w");
    if (file == NULL) {
        perror("Error: Failed to open file stream for writing");
        closeShm(shm);
        close(ans_fd);
        return ERROR;
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

    int slaves_needed = MIN(CANT_SLAVES, ((argc-1 + FILES_PER_SLAVE-1) / FILES_PER_SLAVE));

    for(int i=0; i<slaves_needed ; i++){
        
        if(pipe(pipefd_parent_write) == -1){
            perror("Error: Could not create pipe for slave\n");
            clean_resources_pipe(file, ans_fd, shm, i-1, i, childs_pipe_fds_read, childs_pipe_fds_write);
            return ERROR;
        }
        childs_pipe_fds_write[i] = pipefd_parent_write[PIPE_WRITE]; 

        if(pipe(pipefd_parent_read) == -1){
            perror("Error: Could not create pipe for slave\n");
            clean_resources_pipe(file, ans_fd, shm, i, i, childs_pipe_fds_read, childs_pipe_fds_write);
            return ERROR;
        }
        
        int read_fd = pipefd_parent_read[PIPE_READ];
        childs_pipe_fds_read[i] = read_fd;
       
        if(read_fd > highest_read_fd){
            highest_read_fd = read_fd;  
        }
 

        pid = fork();
        if (pid == -1) {
            perror("Error: could not fork");
            cleanResources(file, ans_fd, shm, i, i-1, childs_pipe_fds_read, childs_pipe_fds_write);
            return ERROR;
        }

        if (pid == 0) {
            if(set_up_slave(i, childs_pipe_fds_read, childs_pipe_fds_write, pipefd_parent_read, pipefd_parent_write) != 0){
                return ERROR; //@TODO deberia liberar algo?/ algo mas ?
            }
            execve(pathname, argv_, envp_);
            perror("Error: Could not excecute ./slave");
            return ERROR;  //@todo deberia liberar algo?
        }
        //Soy el padre:
        close_fd(pipefd_parent_read[PIPE_WRITE]);
        close_fd(pipefd_parent_write[PIPE_READ]);
    }

    //Ya tengo todos los SLAVES creados


    int files_sent = 0;

    for(int i=0; i<slaves_needed && files_sent+1 < argc ; i++){
        for(int j=0; j<FILES_PER_SLAVE && files_sent+1 < argc ; j++ ){
            if(send_file(childs_pipe_fds_write[i],argv[1+files_sent++]) != 0){
                perror("File path is longer than max");
                exit(EXIT_FAILURE);
            }
        } 
    }
        
    //Me quedan archivos para mandar
    int files_read = 0;
    char string_from_fd[BUFFER_SIZE];

    while (files_read < argc - 1) {

        select_function_setup(&readfds, slaves_needed, childs_pipe_fds_read);
        int fds_ready_cant = select(highest_read_fd + 1, &readfds, NULL, NULL, NULL);
        if (fds_ready_cant == -1) {
            perror("select error");
            exit(EXIT_FAILURE);
        }

        int buff_len = 0;
        int count = 0;
        for(int i = 0; i < slaves_needed && fds_ready_cant > 0; i++) {
            if (FD_ISSET(childs_pipe_fds_read[i], &readfds)) {
                read_aux(childs_pipe_fds_read[i], string_from_fd);  
                count = count_newline_strlen(string_from_fd, &buff_len);
                files_read += count;
                writeShm(string_from_fd, shm, buff_len);
                fprintf(file, "%s", string_from_fd); 
                resend_files_to_slave(&files_sent, count, childs_pipe_fds_write, i, argc, argv);
                fds_ready_cant--;
            } 
        }
    }

    char EOF_BUFF[1] = {'\0'};
    writeShm(EOF_BUFF, shm, 0);    
    
    fflush(file);
   cleanResources(file, ans_fd,  shm,  slaves_needed, slaves_needed,  childs_pipe_fds_read,  childs_pipe_fds_write );
   return 0;
}

