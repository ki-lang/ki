
#include <stdbool.h>
#include <sys/types.h>

typedef struct ki_file_stats ki_file_stats;
typedef struct ki_poll_listener ki_poll_listener;
typedef struct ki_poll_result ki_poll_result;
typedef struct ki_poll_event ki_poll_event;

// structs
struct ki_file_stats {
    size_t size;    // file size in bytes
    size_t mtime_s; // last modified in nanoseconds
    bool is_file;
};

struct ki_poll_listener {
    void *poller_;
    size_t key;
    int fd;
    unsigned int state;
};

struct ki_poll_result {
    ki_poll_event *events;
    int count;
};

struct ki_poll_event {
    ki_poll_event *listener;
    unsigned int state;
    /* event state
    0x1  : in
    0x2  : out
    0x4  : err
    0x8  : closed
    0x10 : stopped_reading
    */
};

// enums
enum KI_SIG {
    sig_hup,
    sig_int,
    sig_quit,
    sig_abort,
    sig_kill,
    sig_segv,
    sig_pipe,
    sig_term,
    sig_stop,
    sig_continue,
    sig_child,
    sig_io,
};
enum KI_POLL_EVENTS {
    poll_event_in,
    poll_event_out,
    poll_event_err,
    poll_event_closed,
    poll_event_stopped_reading,
};
enum KI_SOCK_DOMAIN {
    sock_dom_ipv4,
    sock_dom_ipv6,
    sock_dom_file,
};
enum KI_SOCK_CON_TYPE {
    sock_con_type_stream,
};
enum KI_SOCK_OPT {
    sock_opt_reuse_adr,
    sock_opt_keep_alive,
};

// mem
void *ki_os__alloc(size_t size);
void ki_os__free(void *adr);
// sys
char *ki_os__exe_path();
char *ki_os__cwd();
char *ki_os__user_dir();
void ki_os__exit(int code);
void ki_os__signal(int sig, void (*handler)(int));
void ki_os__sleep_ms(unsigned long int msec);
void ki_os__sleep_ns(unsigned long int ns);
void ki_os__raise(int sig);
// io
ssize_t ki_os__fd_read(int fd, void *buf, size_t len);
ssize_t ki_os__fd_write(int fd, void *buf, size_t len);
int ki_os__fd_pipe(int fds[2]);
bool ki_os__fd_close(int fd);
// file
int ki_os__file_open(void *path, int path_len, bool read, bool write);
bool ki_os__file_create(void *path, int path_len, int mode);
bool ki_os__file_write(void *path, int path_len, void *content, size_t len, bool append);
bool ki_os__file_delete(void *path, int path_len);
bool ki_os__file_mkdir(void *path, int path_len);
void ki_os__file_sync();
ki_file_stats *ki_os__file_stats(void *path, int path_len, ki_file_stats *s);
void *ki_os__files_in_dir(void *path, int path_len);
// net
char *ki_os__domain_to_ip(void *domain, int domain_len);
// socket
void *ki_os__socket_create(int ki_domain, int ki_connection_type);
void ki_os__socket_free(void *sock);
int ki_os__socket_get_fd(void *sock);
bool ki_os__socket_set_ipv4(void *sock, char *ip, char *port);
int ki_os__socket_accept(void *sock, char *ip_buffer);
// poll
void *ki_os__poll_init();
void ki_os__poll_free(void *poller);
void ki_os__poll_new_fd(void *poller_, ki_poll_listener *listener);
void ki_os__poll_update_fd(void *poller_, ki_poll_listener *listener, unsigned int state);
void ki_os__poll_remove_fd(void *poller_, ki_poll_listener *listener);
// void ki_os__poll_set_fd(void *poller_, int fd, bool is_new, bool edge_triggered, bool track_in, bool track_out, bool track_err, bool track_closed, bool track_stopped_reading);
// void ki_os__poll_remove_fd(void *poller_, int fd);
ki_poll_result *ki_os__poll_wait(void *poller_, int timeout);
// thread
void *ki_os__thread_create(void *(*handler)(void *), void *data);
// mutex
void *ki_os__mutex_create();
void ki_os__mutex_free(void *mut_);
void ki_os__mutex_lock(void *mut_);
void ki_os__mutex_unlock(void *mut_);
// wait
void *ki_os__signal_wait_create();
void ki_os__signal_wait_free(void *sw_);
void ki_os__signal_wait_wait(void *sw_, int msec, bool (*func)(void *), void *func_arg);
void ki_os__signal_wait_continue(void *sw_);
