
#include "../all.h"

void fc_free(Fc *fc) {
    //
    Array *allocs = fc->allocs;
    int alc = fc->allocs->length;
    for (int i = 0; i < alc; i++) {
        free(array_get_index(allocs, i));
    }
    free(fc);
}

Fc *fc_init(Build *b) {
    //
}
