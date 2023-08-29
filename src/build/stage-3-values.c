
#include "../all.h"

void stage_3_class(Fc *fc, Class *class);
void stage_3_func(Fc *fc, Func *func);

void class_generate_free(Class *class);
void class_generate_deref_props(Class *class);

void stage_3(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 2) {
        printf("# Stage 3 : Read values : %s\n", fc->path_ki);
    }

    for (int i = 0; i < fc->classes->length; i++) {
        Class *class = array_get_index(fc->classes, i);
        if (class->is_generic_base)
            continue;
        stage_3_class(fc, class);
    }
    for (int i = 0; i < fc->funcs->length; i++) {
        Func *func = array_get_index(fc->funcs, i);
        if (b->verbose > 2) {
            printf("> Read func values: %s\n", func->dname);
        }
        stage_3_func(fc, func);
    }

    //
    chain_add(b->stage_4_1, fc);
}

void stage_3_class(Fc *fc, Class *class) {

    if (fc->b->verbose > 2) {
        printf("> Read class values: %s\n", class->dname);
    }

    Allocator *alc = fc->alc;
    Map *props = class->props;
    for (int i = 0; i < props->keys->length; i++) {
        ClassProp *prop = array_get_index(props->values, i);

        Chunk *chunk = prop->value_chunk;
        if (!chunk)
            continue;

        *fc->chunk = *chunk;

        Value *val = read_value(fc, alc, class->scope, false, 0, false);
        val = try_convert(fc, alc, val, prop->type);
        type_check(fc, prop->type, val->rett);

        prop->value = val;
    }

    if (class->type == ct_struct) {
        if (class->is_rc && class->func_deref_props) {
            class_generate_deref_props(class);
        }
        class_generate_free(class);
    }
}

void stage_3_func(Fc *fc, Func *func) {
    //
    Allocator *alc = fc->alc;
    Map *args = func->args_by_name;
    for (int i = 0; i < args->keys->length; i++) {
        Arg *arg = array_get_index(args->values, i);

        Chunk *chunk = arg->value_chunk;
        if (!chunk)
            continue;

        fc->chunk = chunk;

        Value *val = read_value(fc, alc, fc->scope, false, 0, false);
        val = try_convert(fc, alc, val, arg->type);
        type_check(fc, arg->type, val->rett);

        arg->value = val;
    }
}
