
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
