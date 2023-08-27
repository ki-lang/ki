

#include "../all.h"

void stage_2_4(Fc *fc) {
    // Update properties
    Build *b = fc->b;
    if (b->verbose > 2) {
        printf("# Stage 2.4 : Update class properties : %s\n", fc->path_ki);
    }

    Array *classes = fc->classes;
    for (int i = 0; i < classes->length; i++) {
        Class *class = array_get_index(classes, i);
        if (class->is_generic_base)
            continue;

        stage_2_4_class_props_update(fc, class);
    }

    chain_add(b->stage_2_5, fc);
}

void stage_2_4_class_props_update(Fc *fc, Class *class) {
    //
    Build *b = fc->b;
    if (!class->is_rc)
        return;
    Array *types = class->refers_to_types;
    Array *names = class->refers_to_names;
    for (int i = 0; i < types->length; i++) {
        Type *type = array_get_index(types, i);
        if (type->array_of) {
            type = type->array_of;
        }
        Class *pclass = type->class;
        if (!pclass) {
            continue;
        }
        if (!pclass->is_circular && !pclass->track_ownership) {
            type->shared_ref = false;
        }
        if (class->is_rc && type->shared_ref) {
            Array *prop_names = array_make(b->alc, 10);
            stage_2_3_circular_find(class, pclass, prop_names);
            // Circular error
            // Add first property to list
            char *pname = array_get_index(names, i);
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