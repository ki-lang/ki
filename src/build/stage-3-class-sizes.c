
#include "../all.h"

void stage_3_circular(Class *class);

void stage_3(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 2) {
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

        // Detect if class is circular
        stage_3_circular(class);
    }

    //
    chain_add(b->stage_4, fc);
}

bool stage_3_circular_find(Class *find, Class *in) {
    //
    if (in->circular_checked) {
        return false;
    }
    in->circular_checked = true;
    Map *props = in->props;
    for (int i = 0; i < props->values->length; i++) {
        ClassProp *prop = array_get_index(props->values, i);
        Class *pclass = prop->type->class;
        if (pclass && pclass->type == ct_struct && pclass->is_rc) {
            if (pclass == find) {
                in->circular_checked = false;
                return true;
            }
            bool check = stage_3_circular_find(find, pclass);
            if (check) {
                in->circular_checked = false;
                return true;
            }
        }
    }
    in->circular_checked = false;
    return false;
}

void stage_3_circular(Class *class) {
    //
    if (class->type == ct_struct && class->is_rc) {
        class->circular = stage_3_circular_find(class, class);
    }
}
