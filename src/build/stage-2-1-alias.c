
#include "../all.h"

void stage_2_1(Fc *fc) {
    // Aliasses
    Build *b = fc->b;
    if (b->verbose > 2) {
        printf("# Stage 2.1 : Read aliasses : %s\n", fc->path_ki);
    }

    for (int i = 0; i < fc->aliasses->length; i++) {
        Alias *a = array_get_index(fc->aliasses, i);
        fc->chunk = a->chunk;

        Idf *idf_a = NULL;
        if (a->type == alias_id) {
            Idf *idf = read_idf(fc, fc->scope, true, true);
            idf_a = idf_init(fc->alc, idf->type);
            idf_a->item = idf->item;
        }
        map_set(fc->nsc->scope->identifiers, a->name, idf_a);
        if (fc->is_header) {
            map_set(fc->scope->identifiers, a->name, idf_a);
        }
    }

    chain_add(b->stage_2_2, fc);
}

// void stage_2_1(Fc *fc) {
//     //
//     Build *b = fc->b;
//     Allocator *alc = fc->alc;
//     Array *extends = fc->extends;

//     for (int i = 0; i < extends->length; i++) {
//         Extend *ex = array_get_index(extends, i);

//         fc->chunk = ex->chunk_type;
//         Type *type = read_type(fc, alc, fc->scope, true, true, rtc_default);
//         if (!type->class) {
//             sprintf(fc->sbuf, "This type has no class/struct");
//             fc_error(fc);
//         }
//         Class *class = type->class;

//         fc->chunk = ex->chunk_body;
//         Scope *class_scope = class->scope;
//         Scope *extend_scope = scope_init(fc->alc, sct_default, fc->scope, false);
//         class->scope = extend_scope;
//         stage_2_class_props(fc, class, false, true);
//         class->scope = class_scope;
//     }

//     for (int i = 0; i < fc->funcs->length; i++) {
//         Func *func = array_get_index(fc->funcs, i);
//         if (!func->chunk_args)
//             continue;
//         if (b->verbose > 2) {
//             printf("> Scan func types: %s\n", func->dname);
//         }
//         stage_2_func(fc, func);
//     }

//     for (int i = 0; i < fc->classes->length; i++) {
//         Class *class = array_get_index(fc->classes, i);
//         if (class->is_generic_base)
//             continue;
//         if (b->verbose > 2) {
//             printf("> Class type check internal functions: %s\n", class->dname);
//         }
//         stage_2_class_type_checks(fc, class);
//     }

//     //
//     chain_add(b->stage_3, fc);
// }
