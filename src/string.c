
#include "all.h"

Str *str_make(Allocator *alc, char *data) {
    Str *result = al(alc, sizeof(Str));
    result->alc = alc;
    result->length = strlen(data);
    result->mem_size = result->length + 10;
    result->data = al(alc, result->mem_size);
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
        void *new_data = al(str->alc, str->mem_size);
        memcpy(new_data, str->data, str->length);
        memcpy(new_data + str->length, add->data, add->length);
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
        void *new_data = al(str->alc, str->mem_size);
        memcpy(new_data, str->data, str->length);
        str->data = new_data;
    }
    char *adr = str->data + str->length;
    *adr = ch;
    str->length = new_length;
}

void str_append_chars(Str *str, char *add) {
    int add_len = strlen(add);
    if (add_len == 0) {
        return;
    }
    int new_length = str->length + add_len;
    bool reloc = str->mem_size < new_length;
    while (str->mem_size < new_length) {
        str->mem_size *= 2;
    }
    if (reloc) {
        void *new_data = al(str->alc, str->mem_size);
        memcpy(new_data, str->data, str->length);
        memcpy(new_data + str->length, add, add_len);
        str->data = new_data;
    } else {
        memcpy(str->data + str->length, add, add_len);
    }
    str->length = new_length;
}

char *str_to_chars(Allocator *alc, Str *str) {
    char *res = alc ? al(alc, str->length + 1) : malloc(str->length + 1);
    memcpy(res, str->data, str->length);
    res[str->length] = '\0';
    return res;
}
