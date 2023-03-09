
#include "../all.h"

void stage_1(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 0) {
        printf("# Stage 1 : %s\n", fc->path_ki);
    }

    //
    chain_add(b->stage_2, fc);
}
