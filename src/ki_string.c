
#include "all.h"

// const:   {string_length:uxx}-1-{...string bytes...}

void *ki_string_create(Allocator *alc, char *content, int len) {
    //
    void *data = al(alc, len + sizeof(size_t) + 1);

    char *adr = data;
    *(int *)adr = len;
    adr += sizeof(size_t);
    *(char *)adr = 1;
    adr += 1;
    int i = 0;
    while (i < len) {
        char ch = content[i];
        i++;
        *(char *)adr = ch;
        adr += 1;
    }
    return data;
}
