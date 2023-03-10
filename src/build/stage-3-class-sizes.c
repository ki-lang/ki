
#include "../all.h"

void stage_3(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 0) {
        printf("# Stage 3 : Class sizes : %s\n", fc->path_ki);
    }

    Array *checks = fc->class_size_checks;

    while (true) {

        bool did_fix_any = false;
        bool had_missing_size = false;
        Class *last = NULL;

        for (int i = 0; i < checks->length; i++) {
            Class *class = array_get_index(checks, i);
            if (!class)
                continue;

            if (b->verbose > 1) {
                printf("👀 Check class size : %s\n", class->dname);
            }

            bool check = class_check_size(class);

            if (check) {
                array_set_index(checks, i, NULL);
                did_fix_any = true;
            } else {
                last = class;
                had_missing_size = true;
            }
        }
        if (did_fix_any && last != NULL) {
            sprintf(b->sbuf, "Compiler could not figure out all type sizes");
            die(b->sbuf);
        }
        if (!had_missing_size) {
            break;
        }
    }

    Array *classes = fc->classes;
    for (int i = 0; i < classes->length; i++) {

        Class *class = array_get_index(classes, i);

        if (class->is_generic_base) {
            continue;
        }
        if (class->size == 0) {
            sprintf(b->sbuf, "Missing class size: %s", class->dname);
            die(b->sbuf);
        }
    }

    //
    chain_add(b->stage_4, fc);
}
