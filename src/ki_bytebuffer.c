
#include "all.h"

KiByteBuffer *ki_bytebuffer_create(Allocator *alc) {
    //
    KiByteBuffer *buf = al(alc, sizeof(KiByteBuffer));
    buf->length = 0;
    buf->size = 50;
    buf->data = malloc(sizeof(50));
    return buf;
}

char *ki_bytebuffer_to_chars(Allocator *alc, KiByteBuffer *buf) {
    //
    char *res = al(alc, buf->length + 1);
    int i = 0;
    char *data = buf->data;
    while (i < buf->length) {
        res[i] = data[i];
        i++;
    }
    res[i] = '\0';
    return res;
}
