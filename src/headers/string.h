
#ifndef _H_STR
#define _H_STR

typedef struct Str {
    int length;
    int mem_size;
    void *data;
} Str;

Str *str_make(char *);
void str_append(Str *, Str *);
void str_append_char(Str *, char);
void str_append_chars(Str *, char *);
char *str_to_chars(Str *);
void free_str(Str *);

#endif