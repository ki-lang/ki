
#ifndef _H_ALL
#define _H_ALL

#ifdef _WIN32
#include <windows.h>
#else
// #include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
// #include <sys/wait.h>
#include <dirent.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "libs/cJSON.h"
#include "libs/httpclient.h"

#define KI_PATH_MAX 4096
#define KI_TOKEN_MAX 256
#define KI_VERSION "0.1.0"

#define max_num(x, y) (((x) >= (y)) ? (x) : (y))
#define min_num(x, y) (((x) <= (y)) ? (x) : (y))

typedef struct Allocator Allocator;
typedef struct AllocatorBlock AllocatorBlock;

#include "headers/array.h"
#include "headers/map.h"
#include "headers/string.h"

#include "headers/files.h"

#include "headers/enums.h"
#include "headers/structs.h"

#include "headers/functions.h"

#endif