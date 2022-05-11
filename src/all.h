
#ifdef H_ALL
#else
#define H_ALL 1

#ifdef WIN32
#include <windows.h>
#else
#include <sys/resource.h>
#include <sys/time.h>
#endif

#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> // might be linux only?
//#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#define KI_PATH_MAX 4096
#define KI_TOKEN_MAX 256

#include "headers/array.h"
#include "headers/map.h"
#include "headers/string.h"
#include "libs/cJSON.h"
#include "libs/nxjson.h"
//
#include "headers/config.h"
//
#include "headers/cmd-build-structs.h"
//
#include "headers/cmd-build.h"
#include "headers/cmd-cache.h"
#include "headers/cmd-cfg.h"
#include "headers/cmd-pkg.h"
#include "headers/commands.h"
#include "headers/files.h"
#include "headers/functions.h"
#include "headers/globals.h"
#include "headers/syntax.h"

#endif
