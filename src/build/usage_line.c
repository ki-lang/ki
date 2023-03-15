
#include "../all.h"

UsageLine *usage_line_init(Allocator *alc, Scope *scope, Decl *decl) {
    //
    UsageLine *v = al(alc, sizeof(UsageLine));
    v->init_scope = scope;
    v->first_move = NULL;
    v->follow_up = NULL;
    v->moves_max = 0;
    v->moves_min = 0;
    v->reads_after_move = 0;

    if (scope->usage_keys == NULL) {
        scope->usage_keys = array_make(alc, 8);
        scope->usage_values = array_make(alc, 8);
    }

    int index = array_find(scope->usage_keys, decl, arr_find_adr);
    if (index > -1) {
        array_set_index(scope->usage_keys, index, decl);
        array_set_index(scope->usage_values, index, v);
    } else {
        array_push(scope->usage_keys, decl);
        array_push(scope->usage_values, v);
    }

    return v;
}

void usage_move_var(Allocator *alc, Chunk *chunk, Scope *scope, Decl *decl) {
    //
}

void deref_scope(Allocator *alc, Scope *scope_, Scope *until) {
    // Scope *scope = scope_;
    // while (true) {
    //     Array *decls = scope->decls;
    //     if (decls) {
    //         for (int i = 0; i < decls->length; i++) {
    //             Decl *decl = array_get_index(decls, i);
    //             Type *type = decl->type;
    //             Class *class = type->class;

    //             Scope *sub = scope_init(alc, sct_default, scope_, true);
    //             Value *val = value_init(alc, v_decl, decl, type);
    //             class_ref_change(alc, sub, val, -1);

    //             if (sub->ast->length > 0) {
    //                 array_push(scope_->ast, tgen_deref_decl_used(alc, decl, sub));
    //             }
    //         }
    //     }

    //     // Clear upref slots
    //     scope->upref_slots = map_make(alc);

    //     if (scope == until)
    //         break;
    //     scope = scope->parent;
    // }
}