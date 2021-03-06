
#include "all.h"

void die(char *msg) {
    printf("Error: %s\n", msg);
    exit(1);
}
void die_token(char *msg, char *token) {
    printf("Error: ");
    printf(msg, token);
    printf("\n");
    exit(1);
}

bool ends_with(const char *str, const char *suffix) {
    int str_len = strlen(str);
    int suffix_len = strlen(suffix);

    return (str_len >= suffix_len) && (0 == strcmp(str + (str_len - suffix_len), suffix));
}

char *rand_string(char *str, int size) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMOPQRSTUVWXYZ";
    if (size) {
        --size;
        for (int n = 0; n < size; n++) {
            int key = rand() % (int)(sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
    return str;
}

int atoi(const char *str) {
    int num = 0;
    int i = 0;
    bool isNegetive = false;
    if (str[i] == '-') {
        isNegetive = true;
        i++;
    }
    while (str[i] && (str[i] >= '0' && str[i] <= '9')) {
        num = num * 10 + (str[i] - '0');
        i++;
    }
    if (isNegetive)
        num = -1 * num;
    return num;
}

void prepend(char *s, const char *t) {
    size_t len = strlen(t);
    memmove(s + len, s, strlen(s) + 1);
    memcpy(s, t, len);
}

Array *explode(char *part, char *content) {
    char *copy = strdup(content);
    Array *res = array_make(2);
    char *token = strtok(copy, part);
    while (token != NULL) {
        array_push(res, token);
        token = strtok(NULL, part);
    }
    // for (int i = 0; i < res->length; i++) {
    //     char *x = array_get_index(res, i);
    //     printf(" %s\n", x); // printing each token
    // }
    return res;
}

void exec_simple(char *cmd, char *output) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    FILE *fp;
    char buf[1035];

    fp = popen(cmd, "r");
    if (fp == NULL) {
        die_token("Failed to run command: %s", cmd);
    }

    strcpy(output, "");
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        strcat(output, buf);
    }

    pclose(fp);
#else
    int link[2];
    pid_t pid;
    char out[4096];

    if (pipe(link) == -1)
        die("pipe");

    if ((pid = fork()) == -1)
        die("fork");

    if (pid == 0) {
        dup2(link[1], STDOUT_FILENO);
        dup2(link[1], STDERR_FILENO);
        close(link[0]);
        close(link[1]);
        system(cmd);
        // execl("/bin/sh", "/bin/sh", "-c", cmd, (char *)0);
        exit(1);
    } else {
        close(link[1]);
        int nbytes = read(link[0], out, sizeof(out));
        out[nbytes] = 0;
        if (output)
            strcpy(output, out);
        // printf("Output: (%.*s)\n", nbytes, out);
        // wait(NULL);
    }
#endif
}

bool check_installed_git() {
    char out[4096];
    exec_simple("git --version", out);
    return starts_with(out, "git version");
}
