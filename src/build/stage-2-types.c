
#include "../all.h"

void stage_2(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 0) {
        printf("# Stage 2 : %s\n", fc->path_ki);
    }

    //
    chain_add(b->stage_3, fc);
}
