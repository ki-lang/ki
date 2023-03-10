
#include "../all.h"

void stage_5_class(Fc *fc, Class *class);
void stage_5_func(Fc *fc, Func *func);

void stage_5(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 0) {
        printf("# Stage 5 : Read values : %s\n", fc->path_ki);
    }

    for (int i = 0; i < fc->classes->length; i++) {
        Class *class = array_get_index(fc->classes, i);
        if (b->verbose > 1) {
            printf("> Read class values: %s\n", class->dname);
        }
        stage_5_class(fc, class);
    }
    for (int i = 0; i < fc->funcs->length; i++) {
        Func *func = array_get_index(fc->funcs, i);
        if (b->verbose > 1) {
            printf("> Read func values: %s\n", func->dname);
        }
        stage_5_func(fc, func);
    }

    //
    chain_add(b->stage_6, fc);
}

void stage_5_class(Fc *fc, Class *class) {
    //
    if (class->is_generic_base) {
        return;
    }

    Allocator *alc = fc->alc;
    Map *props = class->props;
    for (int i = 0; i < props->keys->length; i++) {
        ClassProp *prop = array_get_index(props->values, i);

        Chunk *chunk = prop->value_chunk;
        if (!chunk)
            continue;

        fc->chunk = chunk;

        Value *val = read_value(fc, alc, class->scope, false, 0);
        // let tval = val.try_convert(fc, class.scope, prop.type);
        // prop.type.compat_check(fc, tval.rett);

        prop->value = val;
    }

    // if class.type == ClassType.struct {
    // 	if class.use_rc {
    // 		class.generate_sub_ref();
    // 	}
    // 	class.generate_free();
    // }
}

void stage_5_func(Fc *fc, Func *func) {
    //
    Allocator *alc = fc->alc;
    Map *args = func->args_by_name;
    for (int i = 0; i < args->keys->length; i++) {
        Arg *arg = array_get_index(args->values, i);

        Chunk *chunk = arg->value_chunk;
        if (!chunk)
            continue;

        fc->chunk = chunk;

        Value *val = read_value(fc, alc, fc->scope, false, 0);
        // let tval = val.try_convert(fc, class.scope, prop.type);
        // prop.type.compat_check(fc, tval.rett);

        arg->value = val;
    }
}