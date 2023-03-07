
#include "all.h"

Str *str_make(char *data) {
    Str *result = malloc(sizeof(Str));
    result->length = strlen(data);
    result->mem_size = result->length + 10;
    result->data = malloc(result->mem_size);
    memcpy(result->data, data, result->length);
    return result;
}

void str_append(Str *str, Str *add) {
    if (add->length == 0) {
        return;
    }
    int new_length = str->length + add->length;
    bool reloc = str->mem_size < new_length;
    while (str->mem_size < new_length) {
        str->mem_size *= 2;
    }
    if (reloc) {
        void *new_data = malloc(str->mem_size);
        memcpy(new_data, str->data, str->length);
        memcpy(new_data + str->length, add->data, add->length);
        free(str->data);
        str->data = new_data;
    } else {
        memcpy(str->data + str->length, add->data, add->length);
    }
    str->length = new_length;
}

void str_append_char(Str *str, char ch) {
    int new_length = str->length + 1;
    bool reloc = str->mem_size < new_length;
    while (str->mem_size < new_length) {
        str->mem_size *= 2;
    }
    if (reloc) {
        str->data = realloc(str->data, str->mem_size);
    }
    char *adr = str->data + str->length;
    *adr = ch;
    str->length = new_length;
}

void str_append_chars(Str *str, char *add) {
    Str *x = str_make(add);
    str_append(str, x);
    free(x);
}

char *str_to_chars(Str *str) {
    char *res = malloc(str->length + 1);
    memcpy(res, str->data, str->length);
    res[str->length] = '\0';
    return res;
}

void free_str(Str *str) {
    free(str->data);
    free(str);
}
