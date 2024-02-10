
#include "../all.h"

void stage_2_1(Fc *fc) {
    // Aliasses
    Build *b = fc->b;
    if (b->verbose > 2) {
        printf("# Stage 2.1 : Read aliasses : %s\n", fc->path_ki);
    }

    unsigned long start = microtime();

    for (int i = 0; i < fc->aliasses->length; i++) {
        Alias *a = array_get_index(fc->aliasses, i);
        fc->chunk = a->chunk;

        Idf *idf = a->idf;
        if (a->type == alias_id) {
            Idf *idf_r = read_idf(fc, fc->scope, true, true);
            idf->type = idf_r->type;
            idf->item = idf_r->item;
        }
    }

    b->time_parse += microtime() - start;

    chain_add(b->stage_2_2, fc);
}
