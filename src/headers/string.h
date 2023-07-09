
#ifndef _H_STR
#define _H_STR

typedef struct Str {
    Allocator *alc;
    int length;
    int mem_size;
    void *data;
    AllocatorBlock *al_block;
} Str;

Str *str_make(Allocator *alc, int mem_size);
void str_append(Str *, Str *);
void str_append_char(Str *, char);
void str_append_chars(Str *, char *);
char *str_to_chars(Allocator *alc, Str *);
void str_to_buf(Str *str, char *res);
void str_clear(Str *str);

#endif