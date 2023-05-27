
#include "all.h"

void die(char *msg) {
    printf("%s\n", msg);
    exit(1);
}

void parse_argv(char **argv, int argc, Array *has_value, Array *args, Map *options) {
    //
    for (int i = 0; i < argc; i++) {
        char *arg = argv[i];
        int index = array_find(has_value, arg, arr_find_str);
        if (index == -1) {
            array_push(args, arg);
            continue;
        }
        i++;
        if (i == argc) {
            break;
        }
        char *value = argv[i];
        map_set(options, arg, value);
    }
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

#ifndef WIN32
int atoi(const char *str) {
    int num = 0;
    int i = 0;
    bool isNegative = false;
    if (str[i] == '-') {
        isNegative = true;
        i++;
    }
    while (str[i] && (str[i] >= '0' && str[i] <= '9')) {
        num = num * 10 + (str[i] - '0');
        i++;
    }
    if (isNegative)
        num = -1 * num;
    return num;
}
#endif

int hex2int(char *hex) {
    int val = 0;
    while (*hex) {
        // get current character then increment
        uint8_t byte = *hex++;
        // transform hex character to the 4bit equivalent number, using the ascii table indexes
        if (byte >= '0' && byte <= '9')
            byte = byte - '0';
        else if (byte >= 'a' && byte <= 'f')
            byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <= 'F')
            byte = byte - 'A' + 10;
        // shift 4 to make space for new digit, and add the 4 bits of the new digit
        val = (val << 4) | (byte & 0xF);
    }
    return val;
}

void sleep_ns(unsigned int ns) {
#ifdef WIN32
    unsigned int ms = ns / 1000;
    if (ms == 0)
        ms = 1;
    Sleep(ms);
#else
    //
    struct timespec ts;
    int res;

    ts.tv_sec = 0;
    ts.tv_nsec = ns;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);
#endif
}

void simple_hash(char *content_, char *buf_) {

    unsigned char *content = (unsigned char *)content_;
    unsigned char *buf = (unsigned char *)buf_;

    const int hash_len = 32;

    memset(buf, '\0', hash_len);

    int res_pos = 0;
    int str_pos = 0;
    unsigned char diff = 0;

    bool end = false;

    while (true) {
        unsigned char str_ch = content[str_pos++];
        if (str_ch == '\0') {
            end = true;
            str_pos = 0;
            continue;
        }

        diff += (str_ch + str_pos) * 0b00010101 + res_pos;
        buf[res_pos++] += str_ch + diff;

        if (res_pos == hash_len) {
            if (end) {
                break;
            }
            res_pos = 0;
        }
    }

    const char *chars = "TMpUivZnQsHw1klS3Ah5d6qr7tjKxJOIEmYP8VgGzcDR0f2uBe4aobWLNCFy9X";

    int i = hash_len;
    while (i > 0) {
        i--;

        const unsigned char str_ch = buf[i];
        diff += (str_ch + i) * 0b0001011 + i;
        buf[i] = chars[(str_ch + diff) % 62];
    }
    buf[hash_len] = '\0';
}

Array *explode(Allocator *alc, char *part, char *content) {
    char *copy = strdup(content);
    Array *res = array_make(alc, 2);
    char *token = strtok(copy, part);
    while (token != NULL) {
        array_push(res, token);
        token = strtok(NULL, part);
    }
    return res;
}

int system_silent(char *cmd) {
    //
    char scmd[2000];
    strcpy(scmd, cmd);
#ifdef WIN32
    strcat(scmd, " > nul");
#else
    strcat(scmd, " > /dev/null");
#endif
    return system(scmd);
}

char *str_replace(char *s, const char *s1, const char *s2) {
    char *p = strstr(s, s1);
    if (p != NULL) {
        size_t len1 = strlen(s1);
        size_t len2 = strlen(s2);
        if (len1 != len2)
            memmove(p + len2, p + len1, strlen(p + len1) + 1);
        memcpy(p, s2, len2);
    }
    return s;
}
