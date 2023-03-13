
#include "../all.h"

Class *class_init(Allocator *alc) {
    //
    Class *class = al(alc, sizeof(Class));
    class->type = ct_struct;
    class->size = 0;
    class->is_rc = true;
    class->must_deref = true;
    class->must_ref = true;
    class->is_signed = false;
    class->packed = true;
    class->is_generic_base = false;
    class->chunk_body = NULL;
    class->props = map_make(alc);
    class->funcs = map_make(alc);
    class->allow_math = false;

    class->func_ref = NULL;
    class->func_deref = NULL;
    class->func_deref_props = NULL;
    class->func_free = NULL;

    return class;
}
ClassProp *class_prop_init(Allocator *alc, Class *class, Type *type) {
    ClassProp *prop = al(alc, sizeof(ClassProp));
    prop->type = type;
    prop->index = class->props->keys->length;
    prop->value = NULL;
    prop->value_chunk = NULL;

    return prop;
}

bool class_check_size(Class *class) {
    // Calculate size (with alignment)
    if (class->type != ct_struct) {
        return true;
    }
    Build *b = class->fc->b;
    int size = 0;
    int largest = 0;
    int propc = class->props->values->length;
    for (int i = 0; i < propc; i++) {
        //
        ClassProp *prop = array_get_index(class->props->values, i);
        int prop_size = type_get_size(b, prop->type);
        if (prop_size == 0) {
            return false;
        }
        if (prop_size > largest) {
            largest = prop_size;
        }
        size += prop_size;
        //
        if (class->packed)
            continue;
        // Calculate padding
        int next_i = i + 1;
        if (next_i < propc) {
            ClassProp *prop = array_get_index(class->props->values, i);
            int next_size = type_get_size(b, prop->type);
            if (next_size == 0) {
                return false;
            }
            int rem = size % next_size;
            if (rem > 0) {
                // Add padding
                size += next_size - rem;
            }
        }
    }
    if (!class->packed) {
        int remain = size % largest;
        size += remain;
    }

    class->size = size;
    // printf("Size: %s | %d\n", class->display_name, size);

    return true;
}

Func *class_define_func(Fc *fc, Class *class, bool is_static, char *name_, Array *args, Type *rett) {
    //
    if (map_get(class->funcs, name_)) {
        return NULL;
    }

    char *name = dups(fc->alc, name_);

    char *gname = al(fc->alc, strlen(name) + strlen(class->gname) + 10);
    sprintf(gname, "%s__%s", class->gname, name);
    char *dname = al(fc->alc, strlen(name) + strlen(class->dname) + 10);
    sprintf(dname, "%s.%s", class->dname, name);

    Func *func = func_init(fc->alc);
    func->fc = fc;
    func->name = name;
    func->gname = gname;
    func->dname = dname;
    func->scope = scope_init(fc->alc, sct_func, fc->scope, true);
    func->scope->func = func;
    func->is_static = is_static;
    if (args)
        func->args = args;
    func->rett = rett;

    array_push(fc->funcs, func);
    map_set(class->funcs, name, func);

    return func;
}

void class_generate_deref_props(Class *class) {
    //
    Func *func = map_get(class->funcs, "__deref_props");
    if (func->chunk_body)
        return;
}
void class_generate_free(Class *class) {
    //
    Func *func = map_get(class->funcs, "__free");
    if (func->chunk_body)
        return;
}
