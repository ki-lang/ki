
#include "../all.h"

void stage_2_3(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 2) {
        printf("# Stage 2.3 : Post work on class properties : %s\n", fc->path_ki);
    }

    unsigned long start = microtime();

    Array *checks = fc->class_size_checks;

    while (true) {

        bool did_fix_any = false;
        bool had_missing_size = false;
        Class *last = NULL;

        for (int i = 0; i < checks->length; i++) {
            Class *class = array_get_index(checks, i);
            if (!class)
                continue;

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
            sprintf(b->sbuf, "Compiler could not figure out the sizes of all types. Most likely due to circular dependency of inline types.");
            build_error(b, b->sbuf);
        }
        if (!had_missing_size) {
            break;
        }
    }

    Array *classes = fc->classes;
    for (int i = 0; i < classes->length; i++) {
        Class *class = array_get_index(classes, i);
        if (class->is_generic_base)
            continue;

        stage_2_3_circular(b, class);

        if (class->size == 0) {
            sprintf(b->sbuf, "Missing class size: %s", class->dname);
            build_error(b, b->sbuf);
        }
    }

    b->time_parse += microtime() - start;

    //
    chain_add(b->stage_2_4, fc);
}

bool stage_2_3_circular_find(Class *find, Class *in, Array *prop_names) {
    //
    if (in->circular_checked) {
        return false;
    }
    if (in == find) {
        return true;
    }
    in->circular_checked = true;
    Array *types = in->refers_to_types;
    Array *names = in->refers_to_names;
    for (int i = 0; i < types->length; i++) {
        Type *type = array_get_index(types, i);
        char *name = array_get_index(names, i);
        Class *pclass = type->class;
        if (!pclass || type->weak_ptr) {
            continue;
        }
        if (pclass->is_rc) {
            if (prop_names)
                array_push(prop_names, name);
            if (pclass == find) {
                in->circular_checked = false;
                return true;
            }
            bool check = stage_2_3_circular_find(find, pclass, prop_names);
            if (check) {
                in->circular_checked = false;
                return true;
            }
            if (prop_names)
                array_pop(prop_names);
        }
    }
    in->circular_checked = false;
    return false;
}

void stage_2_3_circular(Build *b, Class *class) {
    //
    if (!class->is_rc)
        return;

    bool circular = false;
    Array *types = class->refers_to_types;
    Array *names = class->refers_to_names;
    for (int i = 0; i < types->length; i++) {
        Type *type = array_get_index(types, i);
        if (type->array_of) {
            type = type->array_of;
        }
        Class *pclass = type->class;
        if (!pclass || type->weak_ptr) {
            continue;
        }
        if (pclass->is_rc) {
            if (stage_2_3_circular_find(class, pclass, NULL)) {
                circular = true;
                break;
            }
        }
    }
    if (circular) {
        class->is_circular = true;
        class->track_ownership = true;
        // printf("circular: %s\n", class->dname);
    }
}
