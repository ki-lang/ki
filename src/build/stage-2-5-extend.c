
#include "../all.h"

void stage_2_5(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 2) {
        printf("# Stage 2.5 : Extend : %s\n", fc->path_ki);
    }

    Allocator *alc = fc->alc;
    Array *extends = fc->extends;

    for (int i = 0; i < extends->length; i++) {
        Extend *ex = array_get_index(extends, i);

        fc->chunk = ex->chunk_type;
        Type *type = read_type(fc, alc, fc->scope, true, true, rtc_default);
        if (!type->class) {
            sprintf(fc->sbuf, "This type has no class/struct");
            fc_error(fc);
        }
        Class *class = type->class;

        fc->chunk = ex->chunk_body;
        Scope *class_scope = class->scope;
        Scope *extend_scope = scope_init(fc->alc, sct_default, fc->scope, false);
        class->scope = extend_scope;
        stage_2_2_class_read_props(fc, class, false, true);
        class->scope = class_scope;
    }

    chain_add(b->stage_2_6, fc);
}
