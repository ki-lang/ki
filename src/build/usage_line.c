
#include "../all.h"

UsageLine *usage_line_init(Allocator *alc, Scope *scope, Decl *decl) {
    //
    UsageLine *v = al(alc, sizeof(UsageLine));
    v->decl = decl;
    v->scope = scope;
    v->first_move = NULL;
    v->upref_token = NULL;
    v->deref_token = NULL;
    v->ancestors = NULL;
    v->parent = NULL;
    v->clone_from = NULL;
    v->deref_scope = NULL;
    v->moves = 0;
    v->reads_after_move = 0;
    v->read_after_move = false;

    if (scope->usage_keys == NULL) {
        scope->usage_keys = array_make(alc, 8);
        scope->usage_values = array_make(alc, 8);
    }

    int index = array_find(scope->usage_keys, decl, arr_find_adr);
    if (index > -1) {
        UsageLine *prev = array_get_index(scope->usage_values, index);
        v->parent = prev;
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
    Scope *scope_ = scope;
    while (scope) {
        if (scope->usage_keys) {
            int index = array_find(scope->usage_keys, decl, arr_find_adr);
            if (index > -1) {
                return array_get_index(scope->usage_values, index);
            }
        }
        scope = scope->parent;
    }

    printf("Usage line for '%s' not found (compiler bug)\n", decl->name);
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
        if (class && class->must_ref && !decl->disable_rc) {

            Value *v = vgen_value_then_ir_value(alc, val);

            Scope *sub = scope_init(alc, sct_default, scope, true);
            class_ref_change(alc, sub, v, 1);

            val = vgen_value_and_exec(alc, v, sub, true, true);

            ul->ancestors = NULL;
            ul->upref_token = val->item;
            ul->read_after_move = false;
        }

    } else if (vt == v_class_pa) {
        Class *class = val->rett->class;
        if (class && class->must_ref) {
            val = value_init(alc, v_upref_value, val, val->rett);
        }
    } else if (vt == v_global) {
        val = value_init(alc, v_upref_value, val, val->rett);
    } else if (vt == v_cast) {
        Value *on = val->item;
        val->item = usage_move_value(alc, chunk, scope, on);
    } else if (vt == v_or_break) {
        VOrBreak *vob = val->item;
        vob->value = usage_move_value(alc, chunk, scope, vob->value);
    } else if (vt == v_or_value) {
        VOrValue *vov = val->item;
        vov->left = usage_move_value(alc, chunk, scope, vov->left);
        vov->right = usage_move_value(alc, chunk, vov->value_scope, vov->right);
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
            ul->deref_token = NULL;
            ul->parent = NULL;
            ul->clone_from = NULL;
            ul->deref_scope = NULL;
            ul->read_after_move = false;
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

    bool is_loop = false;

    for (int o = 0; o < ancestors->length; o++) {
        Scope *right = array_get_index(ancestors, o);

        if (right->type == sct_loop) {
            is_loop = true;
            break;
        }
    }

    // Array *used_decls = array_make(alc, lkeys->length);

    int i = 0;
    while (i < lkeys->length) {
        Decl *decl = array_get_index(lkeys, i);
        UsageLine *l_ul = array_get_index(lvals, i);
        // Clone left
        UsageLine *new_ul = al(alc, sizeof(UsageLine));
        *new_ul = *l_ul;
        new_ul->ancestors = NULL;
        new_ul->upref_token = NULL;
        new_ul->deref_token = NULL;
        new_ul->parent = NULL;
        new_ul->clone_from = is_loop ? NULL : l_ul;
        new_ul->deref_scope = NULL;
        new_ul->read_after_move = false;

        array_set_index(lvals, i, new_ul);

        // // Check for uses
        // for (int o = 0; o < ancestors->length; o++) {
        //     break;
        //     Scope *right = array_get_index(ancestors, o);
        //     Array *rkeys = right->usage_keys;
        //     Array *rvals = right->usage_values;

        //     UsageLine *r_ul = array_get_index(rvals, i);

        //     if (r_ul->moves > l_ul->moves) {
        //         array_push(used_decls, decl);
        //         break;
        //     }
        // }
        i++;
    }

    // Compare old usage line with the new youngest usage line
    i = 0;
    while (i < lkeys->length) {
        Decl *decl = array_get_index(lkeys, i);
        UsageLine *new_ul = array_get_index(lvals, i);

        // bool used = array_contains(used_decls, decl, arr_find_adr);

        // if (!right_returned && !used) {
        //     i++;
        //     continue;
        // }

        for (int o = 0; o < ancestors->length; o++) {
            Scope *right = array_get_index(ancestors, o);
            Array *rkeys = right->usage_keys;
            Array *rvals = right->usage_values;

            UsageLine *r_ul = array_get_index(rvals, i);

            new_ul->moves = max_num(new_ul->moves, r_ul->moves);
            new_ul->reads_after_move = max_num(new_ul->reads_after_move, r_ul->reads_after_move);

            if (!new_ul->first_move)
                new_ul->first_move = r_ul->first_move;

            if (!is_loop) {
                Type *type = decl->type;
                Class *class = type->class;
                // if (class && (class->must_deref || class->must_ref)) {
                // if (uesd && class && (class->must_deref || class->must_ref)) {
                if (!new_ul->ancestors)
                    new_ul->ancestors = array_make(alc, 8);
                array_push(new_ul->ancestors, r_ul);
                // }
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
    if (class && (class->must_deref || class->must_ref) && !decl->disable_rc) {

        if (ul->upref_token && !ul->read_after_move) {
            // Disable upref
            ul->upref_token->enable_exec = false;

        } else if (ul->ancestors && !ul->read_after_move) {

            //
            for (int i = 0; i < ul->ancestors->length; i++) {
                UsageLine *anc = array_get_index(ul->ancestors, i);
                if (!anc->scope->did_return) {
                    end_usage_line(alc, anc);
                }
            }

            // Simplify algorithm of previous scopes
            if (ul->clone_from) {
                bool all_deref = true;
                bool read_after_move = false;
                for (int i = 0; i < ul->ancestors->length; i++) {
                    UsageLine *anc = array_get_index(ul->ancestors, i);
                    UsageLine *oldest = anc;
                    while (oldest->parent)
                        oldest = oldest->parent;
                    if (!oldest->deref_token) {
                        all_deref = false;
                    }
                    if (oldest->read_after_move) {
                        read_after_move = true;
                    }
                }
                if (all_deref && !read_after_move) {
                    for (int i = 0; i < ul->ancestors->length; i++) {
                        UsageLine *anc = array_get_index(ul->ancestors, i);
                        UsageLine *oldest = anc;
                        while (oldest->parent)
                            oldest = oldest->parent;
                        oldest->deref_token->enable = false;
                    }
                    end_usage_line(alc, ul->clone_from);
                }
            }

        } else {
            // Add deref token
            if (class->must_deref) {
                if (decl->type->is_strict) {
                    Scope *sub = scope_init(alc, sct_default, ul->scope, true);
                    Value *val = value_init(alc, v_decl, decl, type);
                    class_free_value(alc, sub, val);
                    Token *t = tgen_exec(alc, sub, true);
                    array_push((ul->deref_scope ? ul->deref_scope : ul->scope)->ast, t);
                    ul->deref_token = t->item;
                } else {
                    Scope *sub = scope_init(alc, sct_default, ul->scope, true);
                    Value *val = value_init(alc, v_decl, decl, type);
                    class_ref_change(alc, sub, val, -1);
                    Token *t = tgen_exec(alc, sub, true);
                    array_push((ul->deref_scope ? ul->deref_scope : ul->scope)->ast, t);
                    // array_push(ul->scope->ast, t);
                    ul->deref_token = t->item;
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
                    if (!scope->did_return) {
                        end_usage_line(alc, ul);
                    }
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

Scope *usage_create_deref_scope(Allocator *alc, Scope *scope) {
    //
    Array *decls = scope->usage_keys;
    if (decls) {
        Scope *sub = scope_init(alc, sct_default, scope, true);
        for (int i = 0; i < decls->length; i++) {
            UsageLine *ul = array_get_index(scope->usage_values, i);
            ul->deref_scope = sub;
        }
        return sub;
    }
    return NULL;
}
