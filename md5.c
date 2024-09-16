// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "md5.h"

#define PIPE_WRITE 1
#define PIPE_READ 0
#define MODE 0666
#define MIN(a,b) (((a) < (b)) ? (a):(b))
#define MAX(a,b) (((a) > (b)) ? (a):(b))
#define ERROR 1


static inline int send_initial_files(int * files_sent, int slaves_needed, int pipe_path_to_slave[], int argc, char * argv[]);
static inline int manage_slaves_and_write_output(int * files_read, int fds_ready_cant, int slaves_needed, fd_set * select_set, int * files_sent, int pipe_hash_to_master[], int pipe_path_to_slave[],int argc, char * argv[], FILE * file, shared_memory_adt shm );
static inline int resend_files_to_slave(int * files_sent, int cant_to_send, int pipe_path_to_slave[], int slave_index, int argc, char * argv[]);
static inline void select_function_setup(fd_set * readfds, int slaves_needed, int pipe_hash_to_master[]);
static inline int read_from_slave(int fd, char * buffer);

static inline int count_newline_strlen(char *str, int * len);
static inline int send_file_to_slave(int fd, char * buff);

static inline void close_both_fds(int pipe[2]);
static inline void close_fd(int fd);

static inline void create_slaves(int slaves_needed, int * highest_read_fd, int pipe_hash_to_master[], int pipe_path_to_slave[] );
static inline void create_slave_pipes(int pipefd_parent_write[], int pipefd_parent_read[], int i, int * highest_read_fd, int pipe_path_to_slave[], int pipe_hash_to_master[]);
static inline int set_up_slave(int slave_num, int pipe_hash_to_master[], int pipe_path_to_slave[], int pipefd_parent_read[], int pipefd_parent_write[]);
static inline void execute_slave(int i, int pipe_hash_to_master[], int pipe_path_to_slave[], int pipefd_parent_read[], int pipefd_parent_write[]);

static void clean_resources(FILE * file,  shared_memory_adt shm, int slaves_to_close_fd, int childs_to_wait, int pipe_hash_to_master[], int pipe_path_to_slave[]);


int main(int argc, char *argv[]){
    if(argc <= 1){
        perror("Error: No input files specified.\nUsage: ./md5 <file1> [file2 ... file_n]\n");
        return ERROR;
    }

    if(setvbuf(stdout, NULL, _IONBF, 0) != 0){
        perror("Error: Failed to disable buffering for stdout");
        return ERROR;
    }

    int pipe_path_to_slave[CANT_SLAVES], pipe_hash_to_master[CANT_SLAVES];
    int highest_read_fd = 0;
    int slaves_needed = MIN(CANT_SLAVES, ((argc-1 + FILES_PER_SLAVE-1) / FILES_PER_SLAVE));

    create_slaves(slaves_needed, &highest_read_fd, pipe_hash_to_master, pipe_path_to_slave);
    
    shared_memory_adt shm = get_shm(SHM_NAME, true,true);
    if(shm == NULL){
        perror("Error: Could not create shared memory");
        return ERROR;
    }
    
    printf("%s\n", SHM_NAME);  
    sleep(SHM_OPEN_TIME);
    unlink_shm(shm); 
    
    FILE *file = fopen("resultado.txt", "w");
    if (file == NULL) {
        perror("Error: Failed to open file stream for writing");
        close_shm(shm);
        return ERROR;
    }
    
    int files_sent = 0;
    if(send_initial_files(&files_sent, slaves_needed, pipe_path_to_slave, argc, argv) == ERROR){
        clean_resources(file, shm, slaves_needed, slaves_needed, pipe_hash_to_master, pipe_path_to_slave);
        return ERROR;
    }
    
    int files_read = 0;
    fd_set readfds, temp_select_set;
    int fds_ready_cant = 0;
    select_function_setup(&readfds, slaves_needed, pipe_hash_to_master);
    
    while (files_read < argc - 1) {
        temp_select_set = readfds;
        fds_ready_cant = select(highest_read_fd + 1, &temp_select_set, NULL, NULL, NULL);
        if (fds_ready_cant == -1) {
            perror("Error: could not monitor fds from slaves pipe");
            clean_resources(file, shm, slaves_needed, slaves_needed, pipe_hash_to_master, pipe_path_to_slave);
            return ERROR;
        }
        if(manage_slaves_and_write_output(&files_read, fds_ready_cant, slaves_needed, &temp_select_set, &files_sent, pipe_hash_to_master, pipe_path_to_slave, argc, argv, file, shm) == ERROR){
            clean_resources(file, shm, slaves_needed, slaves_needed, pipe_hash_to_master, pipe_path_to_slave);
            return ERROR;
        }
    }
   clean_resources(file, shm, slaves_needed, slaves_needed, pipe_hash_to_master, pipe_path_to_slave);
   return 0;
}


static inline void create_slaves(int slaves_needed, int * highest_read_fd, int pipe_hash_to_master[], int pipe_path_to_slave[] ){
    int pipefd_parent_write[2], pipefd_parent_read[2];
    pid_t pid;

    for(int i=0; i<slaves_needed ; i++){    
        create_slave_pipes(pipefd_parent_write,pipefd_parent_read,i, highest_read_fd, pipe_path_to_slave, pipe_hash_to_master);
        pid = fork();
        if (pid == -1) {
            perror("Error: could not fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            execute_slave(i, pipe_hash_to_master, pipe_path_to_slave, pipefd_parent_read, pipefd_parent_write);
        }
        close_fd(pipefd_parent_read[PIPE_WRITE]);
        close_fd(pipefd_parent_write[PIPE_READ]);
    }
}

static inline void create_slave_pipes(int pipefd_parent_write[], int pipefd_parent_read[], int i, int * highest_read_fd, int pipe_path_to_slave[], int pipe_hash_to_master[]){
    if(pipe(pipefd_parent_write) == -1){
        perror("Error: Could not create pipe for slave\n");
        exit(EXIT_FAILURE);
    }
    pipe_path_to_slave[i] = pipefd_parent_write[PIPE_WRITE]; 
    if(pipe(pipefd_parent_read) == -1){
        perror("Error: Could not create pipe for slave\n");
        exit(EXIT_FAILURE);
    }
    pipe_hash_to_master[i] = pipefd_parent_read[PIPE_READ];
    if(pipefd_parent_read[PIPE_READ] > (*highest_read_fd)){
        *highest_read_fd = pipefd_parent_read[PIPE_READ];
    }
}

static inline void execute_slave(int i, int pipe_hash_to_master[], int pipe_path_to_slave[], int pipefd_parent_read[], int pipefd_parent_write[]){
    const char *pathname = "./slave";
    char *const argv_[] = { "./slave", NULL };  
    char *const envp_[] ={NULL};

    if(set_up_slave(i, pipe_hash_to_master, pipe_path_to_slave, pipefd_parent_read, pipefd_parent_write) != 0){
        exit(EXIT_FAILURE); 
    }
    execve(pathname, argv_, envp_);
    perror("Error: Could not excecute ./slave");
    exit(EXIT_FAILURE);  
}

static inline int set_up_slave(int slave_num, int pipe_hash_to_master[], int pipe_path_to_slave[], int pipefd_parent_read[], int pipefd_parent_write[] ){
            int flag = 0;
            for(int j=0;j<slave_num ; j++){
                close_fd(pipe_hash_to_master[j]);
                close_fd(pipe_path_to_slave[j]);  
            }
            if(dup2(pipefd_parent_read[PIPE_WRITE], STDOUT_FILENO) == -1){
                perror("Error: could not change slave stdout\n");
                flag = ERROR;
            }
            close_both_fds(pipefd_parent_read);
            if(dup2(pipefd_parent_write[PIPE_READ], STDIN_FILENO)){
                perror("Error: could not change slave stdin\n");
                flag = ERROR;
            }
            close_both_fds(pipefd_parent_write);
            return flag;
}

static inline int send_initial_files(int * files_sent, int slaves_needed, int pipe_path_to_slave[], int argc, char * argv[]){
    for(int i=0; (i<slaves_needed) && ((*files_sent)+1 < argc) ; i++){
        for(int j=0; (j<FILES_PER_SLAVE) && ((*files_sent)+1 < argc) ; j++ ){
            if(send_file_to_slave(pipe_path_to_slave[i],argv[(1+(*files_sent)++)]) != 0){
                perror("Error: File path is longer than max");
                return ERROR;
            }
        } 
    }
    return 0;
}

static inline void select_function_setup(fd_set * readfds, int slaves_needed, int pipe_hash_to_master[]){
    FD_ZERO(readfds);
    for(int i = 0; i < slaves_needed; i++) {
        FD_SET(pipe_hash_to_master[i], readfds);
    }
}

static inline int manage_slaves_and_write_output(int * files_read, int fds_ready_cant, int slaves_needed, fd_set * select_set, int * files_sent, int pipe_hash_to_master[], int pipe_path_to_slave[],int argc, char * argv[], FILE * file, shared_memory_adt shm ){
    char string_from_fd[MAX_SLAVE_RESPONSE_LENGHT];
    int buff_len = 0;
    int count = 0;
    for(int i = 0; i < slaves_needed && fds_ready_cant > 0; i++) {
            if (FD_ISSET(pipe_hash_to_master[i], select_set)) {
                if(read_from_slave(pipe_hash_to_master[i], string_from_fd) != 0){
                    fprintf(stderr,"Error: Could not read from fd %d\n", pipe_hash_to_master[i]);
                    return ERROR;
                }
                count = count_newline_strlen(string_from_fd, &buff_len);
                if(count == -1){
                    return ERROR;
                }
                (*files_read) += count;
                if(write_shm(string_from_fd, shm, buff_len) == -1){
                    fprintf(stderr,"Error: Could not write in shared memory\n");
                    return ERROR;
                }
                fprintf(file, "%s", string_from_fd); 
                if(resend_files_to_slave(files_sent, count, pipe_path_to_slave, i, argc, argv) != 0){
                    return ERROR;
                }
                fds_ready_cant--;
            } 
        }
    return 0;
 }

static inline int resend_files_to_slave(int * files_sent, int cant_to_send, int pipe_path_to_slave[], int slave_index, int argc, char * argv[]){
    while((*files_sent < argc -1) && cant_to_send > 0 ){
            if(send_file_to_slave(pipe_path_to_slave[slave_index], argv[1+(*files_sent)++]) != 0){
                return ERROR;
            }
        cant_to_send--;
    }
    return 0;
}

static inline int count_newline_strlen(char *str, int * len) {
    if(str == NULL || len == NULL){
        return -1;
    }
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
static inline int send_file_to_slave(int fd, char * buff){
    int len = strlen(buff);
    if(len >= MAX_PATH_LENGTH){
        perror("Error: File path is longer than max");
        return ERROR;
    }
    buff[len] = '\n';
    if(write(fd, buff, len+1) == -1){
        fprintf(stderr, "Error: Could not write in fd %d\n",fd);
        return ERROR;
    }
    buff[len] = 0;
    return 0;
}

static inline int read_from_slave(int fd, char * buffer){
    ssize_t bytes_read = read(fd, buffer, MAX_SLAVE_RESPONSE_LENGHT-1);
    buffer[bytes_read] = 0;
    if(bytes_read<0){    
        return ERROR;
    }
    return 0;
}

static inline void close_fd(int fd){
    if(close(fd) == -1){
        fprintf(stderr, "Error: could not close fd %d\n", fd);
    }
}

static inline void close_both_fds(int pipe[2]){
    close_fd(pipe[PIPE_READ]);
    close_fd(pipe[PIPE_WRITE]);
}

static void clean_resources(FILE * file,  shared_memory_adt shm, int slaves_to_close_fd, int childs_to_wait, int pipe_hash_to_master[], int pipe_path_to_slave[] ){
    char EOF_BUFF[1] = {'\0'};
    if(write_shm(EOF_BUFF, shm, 0) == -1){
        fprintf(stderr,"Error: Could not write in shared memory\n");
    }
    close_shm(shm);
    fflush(file);
    fclose(file);
    for(int i=0; i<slaves_to_close_fd; i++){
        close_fd(pipe_hash_to_master[i]);
        close_fd(pipe_path_to_slave[i]);
    }
    while(childs_to_wait > 0){
        if(wait(NULL) == -1){
            perror("Error while waiting for child process to terminate");
        }
        childs_to_wait--;
    }
}
