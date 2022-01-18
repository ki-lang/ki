
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

typedef union KI_UNION{
	int i;
	void* ptr;
} KI_UNION;