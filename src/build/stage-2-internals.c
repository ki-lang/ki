
#include "../all.h"

void stage_2_circular(Build *b, Class *class);

void stage_2_internals(Fc *fc) {
    //
    Build *b = fc->b;

    for (int i = 0; i < fc->classes->length; i++) {
        Class *class = array_get_index(fc->classes, i);
        if (class->is_generic_base)
            continue;
        if (b->verbose > 2) {
            printf("> Scan class generate internals: %s\n", class->dname);
        }
        stage_2_circular(b, class);
        stage_2_internals_gen(fc, class);
    }

    //
    chain_add(b->stage_3, fc);
}

void stage_2_internals_gen(Fc *fc, Class *class) {

    // Generate __free / __deref / _RC
    if (class->type == ct_struct) {
        Build *b = fc->b;
        if (class->is_rc) {
            // Define _RC
            Type *type = type_gen(b, b->alc, "u32");
            ClassProp *prop = class_prop_init(b->alc, class, type);
            prop->value = vgen_vint(b->alc, 1, type, false);
            map_set(class->props, "_RC", prop);

            // Define _RC_WEAK
            Type *weak_type = type_gen(b, b->alc, "u32");
            ClassProp *weak_prop = class_prop_init(b->alc, class, weak_type);
            weak_prop->value = vgen_vint(b->alc, 0, weak_type, false);
            map_set(class->props, "_RC_WEAK", weak_prop);

            if (class->circular) {
                // Define _RC_CHECK
                Type *rcc_type = type_gen(b, b->alc, "u32");
                ClassProp *rcc_prop = class_prop_init(b->alc, class, rcc_type);
                rcc_prop->value = vgen_vint(b->alc, 0, rcc_type, false);
                map_set(class->props, "_RC_CHECK", rcc_prop);
                // CC generation
                Type *gen_type = type_gen(b, b->alc, "u8");
                ClassProp *gen_prop = class_prop_init(b->alc, class, gen_type);
                gen_prop->value = vgen_vint(b->alc, 0, gen_type, false);
                map_set(class->props, "_CC_KEEP", gen_prop);
                // CC keep object
                Type *keep_type = type_gen(b, b->alc, "u8");
                ClassProp *keep_prop = class_prop_init(b->alc, class, keep_type);
                keep_prop->value = vgen_vint(b->alc, 0, keep_type, false);
                map_set(class->props, "_CC_CHANGED", keep_prop);

                if (!class->func_cc_check_props)
                    class->func_cc_check_props = class_define_func(fc, class, false, "__cc_check_props", NULL, b->type_void, 0);
                if (!class->func_cc_keep)
                    class->func_cc_keep = class_define_func(fc, class, false, "__cc_keep", NULL, b->type_void, 0);
            }
        }
    }
}

bool stage_2_circular_find(Class *find, Class *in, Array *prop_names) {
    //
    if (in->circular_checked) {
        return false;
    }
    in->circular_checked = true;
    Map *props = in->props;
    for (int i = 0; i < props->values->length; i++) {
        char *name = array_get_index(props->keys, i);
        ClassProp *prop = array_get_index(props->values, i);
        Type *type = prop->type;
        if (type->weak_ptr) {
            continue;
        }
        Class *pclass = type->class;
        if (pclass && pclass->type == ct_struct && pclass->is_rc) {
            array_push(prop_names, name);
            if (pclass == find) {
                in->circular_checked = false;
                return true;
            }
            bool check = stage_2_circular_find(find, pclass, prop_names);
            if (check) {
                in->circular_checked = false;
                return true;
            }
            array_pop(prop_names);
        }
    }
    in->circular_checked = false;
    return false;
}

void stage_2_circular(Build *b, Class *class) {
    //
    if (class->type == ct_struct && class->is_rc) {
        Array *prop_names = array_make(b->alc, 10);
        bool circular = stage_2_circular_find(class, class, prop_names);
        class->circular = circular;
        if (circular && !b->allow_circular) {
            Str *list = str_make(b->alc, 500);
            for (int i = 0; i < prop_names->length; i++) {
                if (i > 0) {
                    str_append_chars(list, " -> ");
                }
                str_append_chars(list, array_get_index(prop_names, i));
            }
            char err[1024];
            sprintf(err, "Circular references are not allowed when using '--no-cc', you should make atleast 1 of the properties a 'weak' references type\nReference loop: %s -> %s (%s)", class->dname, str_to_chars(b->alc, list), class->dname);
            die(err);
        }
    }
}