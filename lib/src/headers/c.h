
#ifdef WIN32
#include <windows.h>
#elif __linux__
//#include <time.h>   // for nanosleep
#endif

//////////
// Memory
//////////

int memcmp(const void *_Buf1,const void *_Buf2,size_t _Size);
void * memcpy(void *_Dst,const void *_Src,size_t _Size);
void * memset(void *_Dst,int _Val,size_t _Size);
void free(void *_Memory);
void * malloc(size_t _Size);
int sizeof(void*);

//////////
// Types
//////////

size_t strlen(char* str);

//////////
// IO
//////////

int printf(char *_Str, void*);
int sprintf(void *buffer, char *_Str, void*);
int puts(char *_Str);

//////////
// Socket
//////////

struct addrinfo {
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    size_t ai_addrlen;
    //socklen_t ai_addrlen;
    void *ai_addr;
    char * ai_canonname;
    struct addrinfo * ai_next;
};

struct in_addr {
    unsigned long s_addr;
};

struct sockaddr_in {
    short            sin_family;
    unsigned short   sin_port;
    struct in_addr   sin_addr;
    // char             sin_zero[8];
    long             sin_zero;
};

// struct sockaddr {
//     unsigned short sa_family;   // address family, AF_xxx
//     char           sa_data[14]; // 14 bytes of protocol address
// };

int getaddrinfo(const char *restrict node, const char *restrict service, const struct addrinfo *restrict hints, struct addrinfo **restrict res);
int socket(int domain, int type, int protocol);
int setsockopt(int socket, int level, int option_name, const void *option_value, int option_len);
int bind(int sock, const void *addr, int addrlen);
int listen(int socket, int backlog);
// int accept(int socket, struct sockaddr *address, int *address_len);
int accept(int socket, void *address, int *address_len);
int recv(int sockfd, void *buf, int len, int flags);
int close(int fd);
int write(int fd, const void *buf, int count);
int shutdown(int sockfd, int how);

///////