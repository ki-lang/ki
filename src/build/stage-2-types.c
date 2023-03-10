
#include "../all.h"

void stage_2_class(Fc *fc, Class *class);
void stage_2_class_props(Fc *fc, Class *class);
void stage_2_func(Fc *fc, Func *func);

void stage_2(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 0) {
        printf("# Stage 2 : %s\n", fc->path_ki);
    }

    for (int i = 0; i < fc->classes->length; i++) {
        Class *class = array_get_index(fc->classes, i);
        if (b->verbose > 1) {
            printf("Scan class types: %s\n", class->gname);
        }
        stage_2_class(fc, class);
    }
    for (int i = 0; i < fc->funcs->length; i++) {
        Func *func = array_get_index(fc->funcs, i);
        if (b->verbose > 1) {
            printf("Scan func types: %s\n", func->gname);
        }
        stage_2_func(fc, func);
    }

    //
    chain_add(b->stage_3, fc);
}

void stage_2_class(Fc *fc, Class *class) {
    //
    if (class->is_generic_base) {
        return;
    }
    //

    //
    stage_2_class_props(fc, class);
}

void stage_2_class_props(Fc *fc, Class *class) {
    //
}

void stage_2_func(Fc *fc, Func *func) {
    //
}