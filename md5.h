#include <sys/select.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   
#include <string.h>
 #include <sys/wait.h>
#include "shm_lib.h"
#include "slave.h"



#ifndef _MD5_H
#define _MD5_H

//@TODO determinar si usar MAX_PATH_LENGHT o hacer un STRLEN en la funcion q hace el write

    #define FILES_PER_SLAVE 2
    #define CANT_SLAVES 10
    #define SHM_NAME "md5_app_shm"
    #define SHM_OPEN_TIME 2

#endif

