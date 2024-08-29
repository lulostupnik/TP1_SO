#include <stdio.h>


#define MAX_FILE_PATH_LENGHT 3
#define FILES_PER_SLAVE 3
#define CANT_SLAVE 5
#define DELIMITER '\n'

//Returns: last index pos
// If error (string longer than max_file_path or no space) --> returns -1
//@Todo verificar condicion de ERROR
int strcat_(char dest[], int dest_size, int start_at, char * str1){
    int i,j;
    
    for(i= start_at, j=0; i<dest_size && str1[j] != 0; i++, j++){
        dest[i] = str1[j];
    }


    if(j >= MAX_FILE_PATH_LENGHT+1 || i == dest_size){
        if(start_at < dest_size){
            dest[start_at] = 0;
        }
        return -1;
    }
    dest[i] = DELIMITER;
    return i+1;
}


//retorna la cantidad que se concatenaron
static int fillBuffer(char destination[], int destination_size ,int cant_string_to_concat_from_source, int from_indx_source, int source_dim, char * source[]){
    int dest_current_dim = 0, ans = 0, i=0;
    for(i=0; i< cant_string_to_concat_from_source && from_indx_source + i<source_dim ; i++){
        ans = strcat_(destination, destination_size, dest_current_dim, source[from_indx_source+i]);
        if(ans == -1){
            return -1;
        }
        dest_current_dim=ans;
    }
    
    if(i== 0){
        return -1;
    }

    if(dest_current_dim < destination_size){
        destination[dest_current_dim] = 0;
    }else{
        return -1;
    }

    return from_indx_source+i;
}



#define BUF_SIZE (FILES_PER_SLAVE*(MAX_FILE_PATH_LENGHT+1)+1)

int main(int argc, char * argv[]){
    char buf[BUF_SIZE];
    for(int i=0 ; i< CANT_SLAVE*FILES_PER_SLAVE && i<argc; i+=FILES_PER_SLAVE){
        if(fillBuffer(buf, BUF_SIZE, FILES_PER_SLAVE, i+1,argc, argv) == -1){
            break;
        }
        printf("%s\n\n\n", buf);
    }
}