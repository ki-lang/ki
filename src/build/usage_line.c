
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

UsageLine *usage_line_get(Scope *scope, Decl *decl) {
    //
    while (scope) {
        if (scope->usage_keys) {
            int index = array_find(scope->usage_keys, decl, arr_find_adr);
            if (index > -1) {
                return array_get_index(scope->usage_values, index);
            }
        }
        scope = scope->parent;
    }

    printf("Usage line not found (compiler bug)\n");
    raise(11);
}

bool is_moved_once(UsageLine *ul) {
    //
    return true;
}

Value *usage_move_value(Allocator *alc, Chunk *chunk, Scope *scope, Value *val) {
    //
    int vt = val->type;
    if (vt == v_var) {
        Var *var = val->item;
        Decl *decl = var->decl;
        UsageLine *ul = usage_line_get(scope, decl);

        ul->moves_max++;
        ul->moves_min++;

        if (!ul->first_move) {
            ul->first_move = chunk_clone(alc, chunk);
        }
    } else {
        // TODO : upref class prop access that have a type class with must_ref = true
        if (vt == v_class_pa) {
            Class *class = val->rett->class;
            if (class && class->must_ref) {
            }
        }
    }

    //
    return val;
}

void deref_scope(Allocator *alc, Scope *scope_, Scope *until) {
    Scope *scope = scope_;
    while (true) {
        Array *decls = scope->usage_keys;
        if (decls) {
            for (int i = 0; i < decls->length; i++) {

                Decl *decl = array_get_index(decls, i);
                UsageLine *ul = array_get_index(scope->usage_values, i);

                Type *type = decl->type;
                Class *class = type->class;

                if (!class->must_deref) {
                    continue;
                }

                Scope *sub = scope_init(alc, sct_default, scope_, true);
                Value *val = value_init(alc, v_decl, decl, type);
                class_ref_change(alc, sub, val, -1);

                if (sub->ast->length > 0) {
                    array_push(scope_->ast, tgen_deref_unless_moved_once(alc, sub, ul));
                }
            }
        }

        if (scope == until)
            break;
        scope = scope->parent;
    }
}