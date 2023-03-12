
#define _GNU_SOURCE

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "../headers/os.h"

////////////
// Memory
////////////

void *ki_os__alloc(size_t size) {
    //
    return malloc(size);
}
void ki_os__free(void *adr) {
    //
    free(adr);
}

////////////
// Sys
////////////

char *ki_os__exe_path_ = NULL;
char *ki_os__exe_path() {
    if (ki_os__exe_path_ != NULL) {
        return ki_os__exe_path_;
    }

    char *buf = malloc(PATH_MAX);
    strcpy(buf, "");
    int len = readlink("/proc/self/exe", buf, 1000);
    buf[len] = '\0';

    ki_os__exe_path_ = buf;
    return ki_os__exe_path_;
}
char *ki_os__cwd() {
    char *cwd = malloc(PATH_MAX);
    if (getcwd(cwd, PATH_MAX) != NULL) {
        return cwd;
    }
    strcpy(cwd, "");
    return cwd;
}
char *ki_os__user_dir() {
    char *dir = malloc(PATH_MAX);
    char *home_dir = getenv("HOME");
    strcpy(dir, home_dir);
    return dir;
}

void ki_os__exit(int code) {
    //
    exit(code);
}
void ki_os__signal(int sig, void (*handler)(int)) {
    //
    int sigc = 0;
    if (sig == sig_hup) {
        sigc = SIGHUP;
    } else if (sig == sig_int) {
        sigc = SIGINT;
    } else if (sig == sig_quit) {
        sigc = SIGQUIT;
    } else if (sig == sig_abort) {
        sigc = SIGABRT;
    } else if (sig == sig_kill) {
        sigc = SIGKILL;
    } else if (sig == sig_segv) {
        sigc = SIGSEGV;
    } else if (sig == sig_pipe) {
        sigc = SIGPIPE;
    } else if (sig == sig_term) {
        sigc = SIGTERM;
    } else if (sig == sig_stop) {
        sigc = SIGSTOP;
    } else if (sig == sig_continue) {
        sigc = SIGCONT;
    } else if (sig == sig_child) {
        sigc = SIGCHLD;
    } else if (sig == sig_io) {
        sigc = SIGIO;
    } else {
        return;
    }

    if (handler) {
        signal(sigc, handler);
    } else {
        signal(sigc, SIG_IGN);
    }
}
void ki_os__raise(int sig) {
    //
    raise(sig);
}
void ki_os__sleep_ms(unsigned long int msec) {
    //
    struct timespec ts;
    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;
    nanosleep(&ts, &ts);
}
void ki_os__sleep_ns(unsigned long int ns) {
    //
    struct timespec ts;
    ts.tv_sec = ns / 1000000000;
    ts.tv_nsec = (ns % 1000000000);
    nanosleep(&ts, &ts);
}

////////////
// FD
////////////

// -2 = EAGAIN | EWOULDBLOCK
ssize_t ki_os__fd_read(int fd, void *buf, size_t len) {
    //
    ssize_t byte_count = read(fd, buf, len);
    if (byte_count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return -2;
    }
    return byte_count;
}
// -2 = EAGAIN | EWOULDBLOCK
ssize_t ki_os__fd_write(int fd, void *buf, size_t len) {
    //
    ssize_t byte_count = write(fd, buf, len);
    if (byte_count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return -2;
    }
    return byte_count;
}
int ki_os__fd_pipe(int fds[2]) {
    // return -1 for error
    return pipe(fds);
}
bool ki_os__fd_close(int fd) {
    //
    int res = close(fd);
    return res != -1;
}

////////////
// Files
////////////

int ki_os__file_open(void *path, int path_len, bool read, bool write) {
    // return -1 for error
    char cpath[path_len + 1];
    memcpy(cpath, path, path_len);
    cpath[path_len] = '\0';
    //
    int fd = open(cpath, 0);
    return fd;
}
bool ki_os__file_create(void *path, int path_len, int mode) {
    // mode e.g. 0644
    char cpath[path_len + 1];
    memcpy(cpath, path, path_len);
    cpath[path_len] = '\0';
    //
    int fd = open(cpath, O_RDWR, mode);
    if (fd < 0) {
        return false;
    }
    write(fd, "", 0);
    fsync(fd);
    close(fd);
    return true;
}
bool ki_os__file_write(void *path, int path_len, void *content, size_t len, bool append) {
    //
    char cpath[path_len + 1];
    memcpy(cpath, path, path_len);
    cpath[path_len] = '\0';
    //
    int flag = O_TRUNC;
    if (append)
        flag = O_APPEND;
    int fd = open(cpath, O_RDWR | O_CREAT | flag, 0744);
    if (fd < 0) {
        return false;
    }
    write(fd, content, len);
    fsync(fd);
    close(fd);
    return true;
}
bool ki_os__file_delete(void *path, int path_len) {
    //
    char cpath[path_len + 1];
    memcpy(cpath, path, path_len);
    cpath[path_len] = '\0';
    return remove(cpath) == 0;
}
bool ki_os__file_mkdir(void *path, int path_len) {
    //
    char cpath[path_len + 1];
    memcpy(cpath, path, path_len);
    cpath[path_len] = '\0';
    return mkdir(cpath, 0700) == 0;
}
void ki_os__file_sync() {
    // Wait for cached writes to sync with disk
    sync();
}
ki_file_stats *ki_os__file_stats(void *path, int path_len, ki_file_stats *s) {
    //
    char cpath[path_len + 1];
    memcpy(cpath, path, path_len);
    cpath[path_len] = '\0';
    //
    struct stat _s;
    int res = stat(cpath, &_s);
    if (res == -1) {
        // int err = errno;
        // printf("ERR: %d | %d %d %d %d %d\n", err, ENAMETOOLONG, ENOENT, ENOMEM, ENOTDIR, EOVERFLOW);
        return NULL;
    }
    s->size = _s.st_size;
    s->mtime_s = _s.st_mtime;
    s->is_file = S_ISREG(_s.st_mode);
    return s;
}
void *ki_os__files_in_dir(void *path, int path_len) {
    //
    char dir[path_len + 1];
    memcpy(dir, path, path_len);
    dir[path_len] = '\0';
    //
    void *result = NULL;
    DIR *d;
    struct dirent *ent;
    if ((d = opendir(dir)) != NULL) {
        int count = 0;
        while ((ent = readdir(d)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            count++;
        }
        result = malloc(sizeof(void *) * (count + 1));
        void **ref = result;
        rewinddir(d);
        while ((ent = readdir(d)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            char *path = malloc(strlen(dir) + strlen(ent->d_name) + 2);
            strcpy(path, dir);
            strcat(path, "/");
            strcat(path, ent->d_name);

            *ref = path;
            ref += 1;
        }
        closedir(d);

        *ref = NULL;
    }
    return result;
}

////////////
// Net
////////////

char *ki_os__domain_to_ip(void *domain, int domain_len) {
    //
    char cpath[domain_len + 1];
    memcpy(cpath, domain, domain_len);
    cpath[domain_len] = '\0';
    //
    struct hostent *he = gethostbyname(domain);
    if (he) {
        struct in_addr **addr_list = (struct in_addr **)he->h_addr_list;
        for (int i = 0; addr_list[i] != NULL; i++) {
            char *ip = ki_os__alloc(32);
            strcpy(ip, inet_ntoa(*addr_list[i]));
            return ip;
        }
    }
    return NULL;
}

////////////
// Sockets
////////////

typedef struct ki_socket {
    int domain;
    int connection_type;
    int fd;
    struct sockaddr_in ipv4_adr;
    struct sockaddr_in6 ipv6_adr;
    struct sockaddr_un file_adr;
} ki_socket;

void *ki_os__socket_create(int ki_domain, int ki_connection_type) {
    //
    int domain = -1;
    if (ki_domain == sock_dom_ipv4) {
        domain = AF_INET;
        // } else if (ki_domain == sock_dom_ipv6) {
        //     domain = AF_INET6;
        // } else if (ki_domain == sock_dom_file) {
        //     domain = AF_LOCAL;
    } else {
        return NULL;
    }
    int con_type = -1;
    if (ki_connection_type == sock_con_type_stream) {
        con_type = SOCK_STREAM;
    } else {
        return NULL;
    }
    int fd = socket(domain, con_type | SOCK_NONBLOCK, 0);
    if (fd == -1) {
        return NULL;
    }

    ki_socket *s = calloc(1, sizeof(ki_socket));
    s->domain = domain;
    s->connection_type = con_type;
    s->fd = fd;

    return s;
}
void ki_os__socket_free(void *sock) {
    //
    free(sock);
}
int ki_os__socket_get_fd(void *sock) {
    //
    ki_socket *s = (ki_socket *)sock;
    return s->fd;
}
bool ki_os__socket_set_ipv4(void *sock, char *ip, char *port) {
    //
    ki_socket *s = (ki_socket *)sock;

    if (s->domain != AF_INET)
        return false;

    struct addrinfo *addrinfo = NULL, hints = {.ai_family = AF_INET, .ai_socktype = SOCK_STREAM, .ai_flags = AI_PASSIVE};
    int e = getaddrinfo(ip, port, &hints, &addrinfo);

    (void)setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int));
    (void)setsockopt(s->fd, SOL_SOCKET, SO_REUSEPORT, (int[]){1}, sizeof(int));

    e = bind(s->fd, addrinfo->ai_addr, addrinfo->ai_addrlen);
    if (e == -1) {
        e = close(s->fd);
        return false;
    } else {
        e = listen(s->fd, INT_MAX);
        if (e == -1)
            return false;
    }

    freeaddrinfo(addrinfo);

    return true;
}
int ki_os__socket_accept(void *sock, char *ip_buffer) {
    //
    ki_socket *s = (ki_socket *)sock;
    int con_fd = -1;
    if (s->domain == AF_INET) {
        // struct sockaddr_in ip_buf;
        // int len = sizeof(ip_buf);
        // con_fd = accept(s->fd, (struct sockaddr *)&ip_buf, &len);
        con_fd = accept4(s->fd, NULL, NULL, SOCK_NONBLOCK);
        // if (ip_buffer) {
        //     strcpy(ip_buffer, inet_ntoa(ip_buf.sin_addr));
        // }
        // } else if (s->domain == AF_INET6) {
        //     struct sockaddr_in6 ip_buf;
        //     int len = sizeof(ip_buf);
        //     con_fd = accept(s->fd, (struct sockaddr *)&ip_buf, &len);
        //     if (ip_buffer) {
        //         char str_buf[48];
        //         inet_ntop(AF_INET, &ip_buf.sin6_addr, str_buf, sizeof(str_buf));
        //         strcpy(ip_buffer, str_buf);
        //     }
        // } else if (s->domain == AF_LOCAL) {
        //     con_fd = accept(s->fd, NULL, NULL);
    } else {
        return -1;
    }

    return con_fd;
}

////////////
// Poll
////////////

typedef struct ki_poller {
    int poll_fd;
    struct epoll_event *events;
    ki_poll_result *result;
} ki_poller;

void *ki_os__poll_init() {
    //
    ki_poller *p = ki_os__alloc(sizeof(ki_poller));
    p->events = malloc(20 * sizeof(struct epoll_event));
    p->poll_fd = epoll_create(1);
    p->result = malloc(sizeof(ki_poll_result));
    p->result->count = 0;
    p->result->events = malloc(20 * sizeof(struct ki_poll_event));
    return (void *)p;
}
void ki_os__poll_free(void *poller_) {
    //
    ki_poller *p = (ki_poller *)poller_;
    if (p->result->events)
        free(p->result->events);
    free(p->result);
    free(p->events);
    free(p);
}
void ki_os__poll_set_fd(void *poller_, int fd, bool is_new, bool edge_triggered, bool track_in, bool track_out, bool track_err, bool track_closed, bool track_stopped_reading) {
    //
    ki_poller *p = (ki_poller *)poller_;

    unsigned int track = 0;
    if (edge_triggered) {
        // track |= EPOLLET;
    }
    if (track_in) {
        track |= EPOLLIN; // | EPOLLET;
    }
    if (track_out) {
        track |= EPOLLOUT;
    }
    if (track_err) {
        track |= EPOLLERR;
    }
    if (track_closed) {
        track |= EPOLLHUP;
    }
    if (track_stopped_reading) {
        track |= EPOLLRDHUP;
    }

    struct epoll_event ev;
    ev.events = track;
    ev.data.fd = fd;
    if (is_new) {
        int res = epoll_ctl(p->poll_fd, EPOLL_CTL_ADD, fd, &ev);
    } else {
        int res = epoll_ctl(p->poll_fd, EPOLL_CTL_MOD, fd, &ev);
        if (errno == ENOENT) {
            // If not found, add it
            int res = epoll_ctl(p->poll_fd, EPOLL_CTL_ADD, fd, &ev);
        }
    }
}
void ki_os__poll_remove_fd(void *poller_, int fd) {
    //
    ki_poller *p = (ki_poller *)poller_;
    struct epoll_event ev;
    ev.events = 0;
    ev.data.fd = fd;
    int res = epoll_ctl(p->poll_fd, EPOLL_CTL_DEL, fd, &ev);
}
ki_poll_result *ki_os__poll_wait(void *poller_, int timeout) {
    // Wait for changes and return event list
    // timeout == -1  --->   wait forever
    ki_poller *p = (ki_poller *)poller_;
    int event_count = epoll_wait(p->poll_fd, p->events, 20, timeout);

    ki_poll_result *result = p->result;
    result->count = event_count;

    for (int i = 0; i < event_count; i++) {
        struct epoll_event *e = p->events + i;
        ki_poll_event *ke = result->events + i;
        ke->fd = e->data.fd;
        unsigned short state = 0;
        int states = e->events;
        if (states & EPOLLIN) {
            state = state | 0x1;
        }
        if (states & EPOLLOUT) {
            state = state | 0x2;
        }
        if (states & EPOLLERR) {
            state = state | 0x4;
        }
        if (states & EPOLLHUP) {
            state = state | 0x8;
        }
        if (states & EPOLLRDHUP) {
            state = state | 0x10;
        }
        ke->state = state;
        ke->os_state = states;
    }

    return result;
}

////////////
// Thread
////////////

void *ki_os__thread_create(void *(*handler)(void *), void *data) {
    //
    pthread_t *t = malloc(sizeof(pthread_t));
    int err = pthread_create(t, NULL, handler, data);
    if (err) {
        return NULL;
    }
    return t;
}

////////////
// Mutex
////////////

void *ki_os__mutex_create() {
    //
    pthread_mutex_t *mut = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mut, NULL);
    return (void *)mut;
}
void ki_os__mutex_free(void *mut_) {
    //
    pthread_mutex_t *mut = (pthread_mutex_t *)mut_;
    free(mut);
}
void ki_os__mutex_lock(void *mut_) {
    //
    pthread_mutex_t *mut = (pthread_mutex_t *)mut_;
    pthread_mutex_lock(mut);
}
void ki_os__mutex_unlock(void *mut_) {
    //
    pthread_mutex_t *mut = (pthread_mutex_t *)mut_;
    pthread_mutex_unlock(mut);
}

////////////
// Wait for signal
////////////

typedef struct ki_signal_wait {
    pthread_mutex_t *mut;
    pthread_cond_t *cond;
    bool is_waiting;
} ki_signal_wait;

void *ki_os__signal_wait_create() {
    //
    pthread_mutex_t *mut = ki_os__mutex_create();
    pthread_cond_t *cond = malloc(sizeof(pthread_cond_t));
    pthread_cond_init(cond, NULL);
    ki_signal_wait *sw = malloc(sizeof(ki_signal_wait));
    sw->mut = mut;
    sw->cond = cond;
    sw->is_waiting = false;
    return (void *)sw;
}
void ki_os__signal_wait_free(void *sw_) {
    //
    ki_signal_wait *sw = (ki_signal_wait *)sw_;
    free(sw);
}
void ki_os__signal_wait_wait(void *sw_, int msec, bool (*func)(void *), void *func_arg) {
    //
    ki_signal_wait *sw = (ki_signal_wait *)sw_;
    ki_os__mutex_lock(sw->mut);

    bool should_lock = true;
    if (func) {
        should_lock = func(func_arg);
    }

    if (should_lock) {
        sw->is_waiting = true;

        if (msec > 0) {
            struct timespec ts;
            ts.tv_sec = msec / 1000;
            ts.tv_nsec = (msec % 1000) * 1000000;
            pthread_cond_timedwait(sw->cond, sw->mut, &ts);
        } else {
            pthread_cond_wait(sw->cond, sw->mut);
        }

        sw->is_waiting = false;
    }

    ki_os__mutex_unlock(sw->mut);
}
void ki_os__signal_wait_continue(void *sw_) {
    //
    ki_signal_wait *sw = (ki_signal_wait *)sw_;
    ki_os__mutex_lock(sw->mut);
    if (sw->is_waiting) {
        pthread_cond_signal(sw->cond);
    }
    ki_os__mutex_unlock(sw->mut);
}

// Delete later
void ki_os__http_date(char *date) {
    time_t t;
    struct tm tm;
    static const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    t = time(NULL);
    (void)gmtime_r(&t, &tm);
    (void)strftime(date, 30, "---, %d --- %Y %H:%M:%S GMT", &tm);
    memcpy(date, days[tm.tm_wday], 3);
    memcpy(date + 8, months[tm.tm_mon], 3);
}
