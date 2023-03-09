
#ifndef _H_STR
#define _H_STR

typedef struct Str {
    Allocator *alc;
    int length;
    int mem_size;
    void *data;
} Str;

Str *str_make(Allocator *, char *);
void str_append(Str *, Str *);
void str_append_char(Str *, char);
void str_append_chars(Str *, char *);
char *str_to_chars(Allocator *alc, Str *);

#endif