
#include "all.h"

Str *str_make(Allocator *alc, int mem_size) {
    if (mem_size == 0) {
        mem_size = 20;
    }
    Str *result = al(alc, sizeof(Str));
    result->alc = alc;
    result->length = 0;
    result->mem_size = mem_size;
    result->al_block = al_private(alc, mem_size);
    result->data = result->al_block->start_adr;
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
        AllocatorBlock *new_block = al_private(str->alc, str->mem_size);
        void *new_data = new_block->start_adr;
        memcpy(new_data, str->data, str->length);
        memcpy(new_data + str->length, add->data, add->length);
        free_block(str->al_block);
        str->al_block = new_block;
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
        AllocatorBlock *new_block = al_private(str->alc, str->mem_size);
        void *new_data = new_block->start_adr;
        memcpy(new_data, str->data, str->length);
        free_block(str->al_block);
        str->al_block = new_block;
        str->data = new_data;
    }
    char *adr = str->data + str->length;
    *adr = ch;
    str->length = new_length;
}

void str_append_chars(Str *str, char *add) {
    int add_len = strlen(add);
    str_append_from_ptr(str, add, add_len);
    // if (add_len == 0) {
    //     return;
    // }
    // int new_length = str->length + add_len;
    // bool reloc = str->mem_size < new_length;
    // while (str->mem_size < new_length) {
    //     str->mem_size *= 2;
    // }
    // if (reloc) {
    //     AllocatorBlock *new_block = al_private(str->alc, str->mem_size);
    //     void *new_data = new_block->start_adr;
    //     memcpy(new_data, str->data, str->length);
    //     memcpy(new_data + str->length, add, add_len);
    //     free_block(str->al_block);
    //     str->al_block = new_block;
    //     str->data = new_data;
    // } else {
    //     memcpy(str->data + str->length, add, add_len);
    // }
    // str->length = new_length;
}
void str_append_from_ptr(Str *str, void *ptr, int len) {
    //
    if (len == 0) {
        return;
    }
    int new_length = str->length + len;
    bool reloc = str->mem_size < new_length;
    while (str->mem_size < new_length) {
        str->mem_size *= 2;
    }
    if (reloc) {
        AllocatorBlock *new_block = al_private(str->alc, str->mem_size);
        void *new_data = new_block->start_adr;
        memcpy(new_data, str->data, str->length);
        memcpy(new_data + str->length, ptr, len);
        free_block(str->al_block);
        str->al_block = new_block;
        str->data = new_data;
    } else {
        memcpy(str->data + str->length, ptr, len);
    }
    str->length = new_length;
}

char *str_to_chars(Allocator *alc, Str *str) {
    char *res = alc ? al(alc, str->length + 1) : malloc(str->length + 1);
    memcpy(res, str->data, str->length);
    res[str->length] = '\0';
    return res;
}

void str_clear(Str *str) {
    //
    str->length = 0;
}
