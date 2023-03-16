
#include "../all.h"

UsageLine *usage_line_init(Allocator *alc, Scope *scope, Decl *decl) {
    //
    UsageLine *v = al(alc, sizeof(UsageLine));
    v->init_scope = scope;
    v->first_move = NULL;
    v->update_chain = NULL;
    v->parent = NULL;
    v->moves_max = 0;
    v->moves_min = 0;
    v->reads_after_move = 0;

    if (scope->usage_keys == NULL) {
        scope->usage_keys = array_make(alc, 8);
        scope->usage_values = array_make(alc, 8);
    }

    int index = array_find(scope->usage_keys, decl, arr_find_adr);
    if (index > -1) {
        UsageLine *parent = array_get_index(scope->usage_values, index);
        v->parent = parent;
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
    // printf("%s\n", ul->init_scope->func->dname);
    // printf("%d,%d\n", ul->moves_min, ul->moves_max);
    return ul->moves_min == 1 && ul->moves_max == 1 && ul->reads_after_move == 0;
}

void usage_read_value(Allocator *alc, Scope *scope, Value *val) {
    //
}

void usage_line_incr_moves(UsageLine *ul, int amount) {
    //
    ul->moves_max += amount;
    ul->moves_min += amount;

    if (ul->update_chain) {
        Array *list = ul->update_chain;
        for (int i = 0; i < list->length; i++) {
            UsageLine *ul = array_get_index(list, i);
            usage_line_incr_moves(ul, amount);
        }
    }
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

            Scope *sub = scope_init(alc, sct_default, scope, true);
            Value *val = value_init(alc, v_decl, decl, type);
            class_ref_change(alc, sub, val, 1);

            if (sub->ast->length > 0) {
                array_push(scope->ast, tgen_exec_unless_moved_once(alc, sub, ul));
            }
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
            ul->update_chain = NULL;
            ul->parent = NULL;

            array_push(keys, decl);
            array_push(vals, ul);
        }

        scope->usage_keys = keys;
        scope->usage_values = vals;
    }

    return scope;
}

void usage_collect_used_decls(Allocator *alc, Scope *left, Scope *right, Array **list_) {
    //
    if (!left->usage_keys) {
        return;
    }

    Array *list = *list_;

    Array *lkeys = left->usage_keys;
    Array *lvals = left->usage_values;
    Array *rkeys = right->usage_keys;
    Array *rvals = right->usage_values;
    int i = 0;
    while (i < lkeys->length) {
        Decl *decl = array_get_index(lkeys, i);

        UsageLine *l_ul = array_get_index(lvals, i);
        UsageLine *r_ul = array_get_index(rvals, i);

        while (r_ul->parent) {
            r_ul = r_ul->parent;
        }

        bool right_moved_once = is_moved_once(r_ul);

        // Moves changed, right side has 1 move, add to array
        if (r_ul->moves_max > l_ul->moves_max && right_moved_once) {
            if (!list) {
                list = array_make(alc, 20);
                *list_ = list;
            }
            array_push_unique(list, decl);
        }
        i++;
    }
}

void usage_merge_scopes(Allocator *alc, Scope *left, Scope *right, Array *used_decls) {
    //
    if (!left->usage_keys) {
        return;
    }

    Array *lkeys = left->usage_keys;
    Array *lvals = left->usage_values;
    Array *rkeys = right->usage_keys;
    Array *rvals = right->usage_values;

    // Compare old usage line with the new youngest usage line
    int i = 0;
    while (i < lkeys->length) {
        Decl *decl = array_get_index(lkeys, i);
        UsageLine *l_ul = array_get_index(lvals, i);
        UsageLine *r_ul = array_get_index(rvals, i);

        // Merge with oldest line
        l_ul->moves_max = max_num(l_ul->moves_max, r_ul->moves_max);
        l_ul->moves_min = min_num(l_ul->moves_min, r_ul->moves_min);
        l_ul->reads_after_move = max_num(l_ul->reads_after_move, r_ul->reads_after_move);

        // Moves changed, right side has 1 move, add to array
        if (used_decls && array_contains(used_decls, decl, arr_find_adr)) {
            l_ul->moves_min = 1;

            // If it's the original line && right side is 0, add a deref to the scope
            if (r_ul->parent == NULL && r_ul->moves_max == 0) {

                Type *type = decl->type;
                Class *class = type->class;
                if (class && class->must_deref) {
                    Scope *sub = scope_init(alc, sct_default, right, true);
                    Value *val = value_init(alc, v_decl, decl, decl->type);
                    class_ref_change(alc, sub, val, -1);
                    array_shift(right->ast, tgen_exec_if_moved_once(alc, sub, r_ul));
                }
            }
        }

        // Add to update chain
        if (!l_ul->update_chain)
            l_ul->update_chain = array_make(alc, 8);

        array_push(l_ul->update_chain, r_ul);

        i++;
    }
}

void deref_scope(Allocator *alc, Scope *scope_, Scope *until) {
    Scope *scope = scope_;
    while (true) {
        Array *decls = scope->usage_keys;
        if (decls) {
            for (int i = 0; i < decls->length; i++) {

                Decl *decl = array_get_index(decls, i);
                UsageLine *ul = array_get_index(scope->usage_values, i);

                if (ul->init_scope != scope)
                    continue;

                Type *type = decl->type;
                Class *class = type->class;

                if (!class->must_deref) {
                    continue;
                }

                Scope *sub = scope_init(alc, sct_default, scope_, true);
                Value *val = value_init(alc, v_decl, decl, type);
                class_ref_change(alc, sub, val, -1);

                if (sub->ast->length > 0) {
                    array_push(scope_->ast, tgen_exec_unless_moved_once(alc, sub, ul));
                }
            }
        }

        if (scope == until)
            break;
        scope = scope->parent;
    }
}