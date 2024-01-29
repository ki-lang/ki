
#ifndef _H_STR
#define _H_STR

typedef struct Str {
    Allocator *alc;
    int length;
    int mem_size;
    void *data;
} Str;

Str *str_make(Allocator *alc, int mem_size);
void str_append(Str *, Str *);
void str_append_char(Str *, char);
void str_append_chars(Str *, char *);
void str_append_from_ptr(Str *str, void *ptr, int len);
char *str_to_chars(Allocator *alc, Str *);
void str_clear(Str *str);
void str_increase_memsize(Str *str, int new_memsize);

#endif