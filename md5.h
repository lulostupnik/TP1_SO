


#ifndef _MD5_H
#define _MD5_H

#include <sys/select.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "shm_lib.h"
#include "slave.h"

#define FILES_PER_SLAVE 2
#define CANT_SLAVES 10
#define SHM_NAME "md5_app_shm"
#define SHM_OPEN_TIME 2

#endif

