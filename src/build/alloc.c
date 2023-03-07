
#include "../all.h"

void *b_alloc(Build *b, size_t size) {
    //
    void *res = malloc(size);
    array_push(b->allocs, res);
    return res;
}
void *fc_alloc(Fc *fc, size_t size) {
    //
    void *res = malloc(size);
    array_push(fc->allocs, res);
    return res;
}
