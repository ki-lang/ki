
#include "../all.h"

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

        stage_3_circular(b, class);

        if (class->size == 0) {
            sprintf(b->sbuf, "Missing class size: %s", class->dname);
            build_error(b, b->sbuf);
        }
    }
    for (int i = 0; i < classes->length; i++) {
        Class *class = array_get_index(classes, i);
        if (class->is_generic_base)
            continue;

        stage_3_shared_circular_refs(b, class);
    }

    //
    chain_add(b->stage_4, fc);
}

bool stage_3_circular_find(Class *find, Class *in, Array *prop_names) {
    //
    if (in->circular_checked) {
        return false;
    }
    if (in == find) {
        return true;
    }
    in->circular_checked = true;
    Map *props = in->props;
    for (int i = 0; i < props->values->length; i++) {
        char *name = array_get_index(props->keys, i);
        ClassProp *prop = array_get_index(props->values, i);
        Type *type = prop->type;
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
            bool check = stage_3_circular_find(find, pclass, prop_names);
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

void stage_3_circular(Build *b, Class *class) {
    //
    if (!class->is_rc)
        return;
    Map *props = class->props;
    bool circular = false;
    for (int i = 0; i < props->values->length; i++) {
        ClassProp *prop = array_get_index(props->values, i);
        Type *type = prop->type;
        Class *pclass = type->class;
        if (!pclass || type->weak_ptr) {
            continue;
        }
        if (pclass->is_rc) {
            if (stage_3_circular_find(class, pclass, NULL)) {
                circular = true;
                break;
            }
        }
    }
    if (circular) {
        class->is_circular = true;
    }
}
void stage_3_shared_circular_refs(Build *b, Class *class) {
    //
    if (!class->is_rc)
        return;
    Map *props = class->props;
    for (int i = 0; i < props->values->length; i++) {
        ClassProp *prop = array_get_index(props->values, i);
        Type *type = prop->type;
        Class *pclass = type->class;
        if (!pclass || type->weak_ptr) {
            continue;
        }
        if (!pclass->is_circular) {
            if (!type_tracks_ownership(type)) {
                type->shared_ref = false;
            }
            continue;
        }
        if (class->is_rc && type->shared_ref) {
            Array *prop_names = array_make(b->alc, 10);
            stage_3_circular_find(class, pclass, prop_names);
            // Circular error
            // Add first property to list
            char *pname = array_get_index(props->keys, i);
            array_shift(prop_names, pname);
            // Make property chain string
            Str *list = str_make(b->alc, 500);
            for (int i = 0; i < prop_names->length; i++) {
                if (i > 0) {
                    str_append_chars(list, " -> ");
                }
                str_append_chars(list, array_get_index(prop_names, i));
            }
            // Show error
            sprintf(b->sbuf, "Property '%s.%s' has a shared reference type, shared reference types are not allowed to loop back to it's own class. We dont allow this because it can result into a memory leak. Use 'weak' references instead for this situation.\nLoop: %s -> %s (%s)", class->dname, pname, class->dname, str_to_chars(b->alc, list), class->dname);
            build_error(b, b->sbuf);
        }
    }
}