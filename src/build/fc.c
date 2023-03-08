
#include "../all.h"

Fc *fc_init(Build *b, char *path_ki) {
    //
    if (!path_ki || !file_exists(path_ki)) {
        sprintf(die_buf, "File not found: %s", path_ki);
        die(die_buf);
    }
    Allocator *alc = b->alc;
    Fc *fc = al(alc, sizeof(Fc));
    fc->b = b;
}
