#ifndef _SLAVE_H
#define _SLAVE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAX_PATH_LENGTH 4096
#define MAX_COMMAND_LENGTH (MAX_PATH_LENGTH + 8 ) // 8 from "md5sum " (null terminated)
#define MAX_PID_DIGITS 7
#define MAX_MD5_OUT_LEN (MD5_HASH_LEN+ MAX_PATH_LENGTH +3)
#define MD5_HASH_LEN 32


#define MAX_SLAVE_RESPONSE_LENGHT (MAX_PID_DIGITS + MAX_MD5_OUT_LEN + 2)

#endif