
#include "../all.h"

void stage_4(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 2) {
        printf("# Stage 4 : Type sizes : %s\n", fc->path_ki);
    }

    Array *types = fc->type_size_checks;
    for (int i = 0; i < types->length; i++) {
        Type *type = array_get_index(types, i);

        int size = type_get_size(b, type);
        if (size == 0) {
            sprintf(fc->sbuf, "Could not calculate size for type (bug)");
            fc_error(fc);
        }
    }

    //
    chain_add(b->stage_5, fc);
}
