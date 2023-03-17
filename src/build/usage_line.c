
#include "../all.h"

UsageLine *usage_line_init(Allocator *alc, Scope *scope, Decl *decl) {
    //
    UsageLine *v = al(alc, sizeof(UsageLine));
    v->decl = decl;
    v->scope = scope;
    v->first_move = NULL;
    v->upref_token = NULL;
    v->ancestors = NULL;
    v->moves = 0;
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

void usage_read_value(Allocator *alc, Scope *scope, Value *val) {
    //
}

void usage_line_incr_moves(UsageLine *ul, int amount) {
    //
    ul->moves += amount;
}

Value *usage_move_value(Allocator *alc, Chunk *chunk, Scope *scope, Value *val) {
    //
    int vt = val->type;
    if (vt == v_var) {
        Var *var = val->item;
        Decl *decl = var->decl;
        UsageLine *ul = usage_line_get(scope, decl);

        // Check if in loop
        bool in_loop = false;
        Scope *s = scope;
        while (true) {
            if (s == decl->scope) {
                break;
            }
            if (s->type == sct_loop) {
                in_loop = true;
                break;
            }
            s = s->parent;
        }

        int incr = in_loop ? 2 : 1;
        usage_line_incr_moves(ul, incr);

        if (!ul->first_move) {
            ul->first_move = chunk_clone(alc, chunk);
        }

        Type *type = val->rett;
        Class *class = type->class;
        if (class && class->must_ref) {

            Value *v = vgen_value_then_ir_value(alc, val);

            Scope *sub = scope_init(alc, sct_default, scope, true);
            class_ref_change(alc, sub, v, 1);

            val = vgen_value_and_exec(alc, v, sub, true, true);

            ul->upref_token = val->item;
        }

    } else if (vt == v_class_pa) {
        Class *class = val->rett->class;
        if (class && class->must_ref) {
            val = value_init(alc, v_upref_value, val, val->rett);
        }
    } else if (vt == v_global) {
        val = value_init(alc, v_upref_value, val, val->rett);
    }

    //
    return val;
}

Scope *usage_scope_init(Allocator *alc, Scope *parent, int type) {
    //
    Scope *scope = scope_init(alc, type, parent, true);

    if (parent->usage_keys) {
        // Clone all usage lines from parent
        Array *keys = array_make(alc, 8);
        Array *vals = array_make(alc, 8);

        Array *par_keys = parent->usage_keys;
        Array *par_vals = parent->usage_values;
        for (int i = 0; i < par_keys->length; i++) {
            Decl *decl = array_get_index(par_keys, i);
            UsageLine *par_ul = array_get_index(par_vals, i);
            UsageLine *ul = al(alc, sizeof(UsageLine));
            *ul = *par_ul;
            ul->ancestors = NULL;
            ul->upref_token = NULL;
            ul->scope = scope;

            array_push(keys, decl);
            array_push(vals, ul);
        }

        scope->usage_keys = keys;
        scope->usage_values = vals;
    }

    return scope;
}

void usage_merge_ancestors(Allocator *alc, Scope *left, Array *ancestors) {
    //
    if (!left->usage_keys) {
        return;
    }

    Array *lkeys = left->usage_keys;
    Array *lvals = left->usage_values;

    Array *used_decls = array_make(alc, lkeys->length);

    int i = 0;
    while (i < lkeys->length) {
        Decl *decl = array_get_index(lkeys, i);
        UsageLine *l_ul = array_get_index(lvals, i);

        l_ul->ancestors = NULL;
        l_ul->upref_token = NULL;

        for (int o = 0; o < ancestors->length; o++) {
            Scope *right = array_get_index(ancestors, o);
            Array *rkeys = right->usage_keys;
            Array *rvals = right->usage_values;

            UsageLine *r_ul = array_get_index(rvals, i);

            if (r_ul->moves > l_ul->moves) {
                array_push(used_decls, decl);
                break;
            }
        }
        i++;
    }

    // Compare old usage line with the new youngest usage line
    i = 0;
    while (i < lkeys->length) {
        Decl *decl = array_get_index(lkeys, i);
        UsageLine *l_ul = array_get_index(lvals, i);

        if (!array_contains(used_decls, decl, arr_find_adr)) {
            i++;
            continue;
        }

        l_ul->ancestors = NULL;
        l_ul->upref_token = NULL;

        for (int o = 0; o < ancestors->length; o++) {
            Scope *right = array_get_index(ancestors, o);
            Array *rkeys = right->usage_keys;
            Array *rvals = right->usage_values;

            UsageLine *r_ul = array_get_index(rvals, i);

            l_ul->moves = max_num(l_ul->moves, r_ul->moves);

            if (!l_ul->first_move)
                l_ul->first_move = r_ul->first_move;

            // l_ul->reads_after_move = max_num(l_ul->reads_after_move, r_ul->reads_after_move);

            // Moves changed, right side has 1 move, add to array
            // if (used_decls && array_contains(used_decls, decl, arr_find_adr)) {

            // If it's the original line && right side is 0, add a deref to the scope
            // if (is_original && r_ul->moves_min == 0) {
            //     o_ul->moves_min = 1;
            //     o_ul->moves_max = 1;

            //     Type *type = decl->type;
            //     Class *class = type->class;
            //     if (class && class->must_deref) {
            //         Scope *sub = scope_init(alc, sct_default, right, true);
            //         Value *val = value_init(alc, v_decl, decl, decl->type);
            //         class_ref_change(alc, sub, val, -1);
            //         array_shift(right->ast, tgen_exec_if_moved_once(alc, sub, l_ul));
            //     }
            // }
            // }

            if (!right->did_return) {
                Type *type = decl->type;
                Class *class = type->class;
                if (class && (class->must_deref || class->must_ref)) {
                    if (!l_ul->ancestors)
                        l_ul->ancestors = array_make(alc, 8);
                    array_push(l_ul->ancestors, r_ul);
                }
            }
        }
        i++;
    }
}

void end_usage_line(Allocator *alc, UsageLine *ul) {
    //
    Decl *decl = ul->decl;
    Type *type = decl->type;
    Class *class = type->class;
    if (class && (class->must_deref || class->must_ref)) {

        if (ul->ancestors) {
            //
            for (int i = 0; i < ul->ancestors->length; i++) {
                UsageLine *anc = array_get_index(ul->ancestors, i);
                end_usage_line(alc, anc);
            }
        } else if (ul->upref_token) {
            // Disable upref
            ul->upref_token->enable_exec = false;
        } else {
            // Add deref token
            if (class->must_deref) {
                if (decl->type->is_strict) {
                    Scope *sub = scope_init(alc, sct_default, ul->scope, true);
                    Value *val = value_init(alc, v_decl, decl, type);
                    class_free_value(alc, sub, val);
                    array_push(ul->scope->ast, token_init(alc, tkn_exec, sub));
                } else {
                    Scope *sub = scope_init(alc, sct_default, ul->scope, true);
                    Value *val = value_init(alc, v_decl, decl, type);
                    class_ref_change(alc, sub, val, -1);
                    array_push(ul->scope->ast, token_init(alc, tkn_exec, sub));
                }
            }
        }
    }
}

void deref_expired_decls(Allocator *alc, Scope *scope) {
    //
    Array *decls = scope->usage_keys;
    if (decls) {
        for (int i = 0; i < decls->length; i++) {
            Decl *decl = array_get_index(decls, i);
            UsageLine *ul = array_get_index(scope->usage_values, i);

            if (decl->scope == scope) {
                end_usage_line(alc, ul);
            }
        }
    }
}

void deref_scope(Allocator *alc, Scope *scope_, Scope *until) {
    Array *decls = scope_->usage_keys;
    if (decls) {
        for (int i = 0; i < decls->length; i++) {
            Decl *decl = array_get_index(decls, i);
            UsageLine *ul = array_get_index(scope_->usage_values, i);

            Scope *scope = scope_;
            while (scope) {

                if (scope == decl->scope) {
                    end_usage_line(alc, ul);
                    break;
                }

                if (scope == until)
                    break;
                scope = scope->parent;
            }
        }
    }
}

void usage_clear_ancestors(Scope *scope) {
    //
    Array *decls = scope->usage_keys;
    if (decls) {
        for (int i = 0; i < decls->length; i++) {
            Decl *decl = array_get_index(decls, i);
            UsageLine *ul = array_get_index(scope->usage_values, i);

            ul->ancestors = NULL;
            ul->upref_token = NULL;
        }
    }
}