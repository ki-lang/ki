
#include "../all.h"

void fc_free(Fc *fc) {
    //
    Array *allocs = fc->allocs;
    int alc = fc->allocs->length;
    for (int i = 0; i < alc; i++) {
        free(array_get_index(allocs, i));
    }
}

Fc *fc_init(Build *b, char *path_ki) {
    //
    if (!path_ki || !file_exists(path_ki)) {
        sprintf(die_buf, "File not found: %s", path_ki);
        die(die_buf);
    }
    Fc *fc = malloc(sizeof(Fc));
    fc->b = b;
    fc->allocs = array_make(100);
}
