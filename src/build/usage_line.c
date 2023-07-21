
#include "../all.h"

UsageLine *usage_line_init(Allocator *alc, Scope *scope, Decl *decl) {
    //
    if (!type_tracks_ownership(decl->type)) {
        return NULL;
    }

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
    v->moves_possible = 0;
    v->reads_after_move = 0;
    v->read_after_move = false;
    v->enable = true;
    v->first_in_scope = false;

    if (scope->usage_keys == NULL) {
        scope->usage_keys = array_make(alc, 8);
        scope->usage_values = array_make(alc, 8);
    }

    int index = decl ? array_find(scope->usage_keys, decl, arr_find_adr) : -1;
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

    return NULL;
}

void usage_read_value(Allocator *alc, Scope *scope, Value *val) {
    //
}

void usage_line_incr_moves(UsageLine *ul, int amount) {
    //
    ul->moves += amount;
    ul->moves_possible += amount;
}

Value *usage_move_value(Allocator *alc, Fc *fc, Scope *scope, Value *val, Type *storage_type) {
    Chunk *chunk = fc->chunk;
    //
    if (storage_type->weak_ptr) {
        Class *class = storage_type->class;
        if (class && class->must_ref) {
            printf("WEAK_1\n");
            Value *v = vgen_value_then_ir_value(alc, val);
            Scope *sub = scope_init(alc, sct_default, scope, true);
            class_ref_change(alc, sub, v, 1, true);
            val = vgen_value_and_exec(alc, v, sub, true, true);
        }
        return val;
    }

    if (!type_tracks_ownership(storage_type)) {
        return val;
    }

    int vt = val->type;
    if (vt == v_decl) {

        Decl *decl = val->item;
        UsageLine *ul = usage_line_get(scope, decl);

        if (ul) {

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

            if (decl->type->strict_ownership && ul->moves > 1) {
                sprintf(fc->sbuf, "Multiple moves of a value. (variable: '%s')", decl->name);
                fc_error(fc);
            }

            if (!ul->first_move) {
                ul->first_move = chunk_clone(alc, chunk);
            }

            Type *type = val->rett;
            Class *class = type->class;
            if (class && class->must_ref) {

                Value *v = vgen_value_then_ir_value(alc, val);

                Scope *sub = scope_init(alc, sct_default, scope, true);
                class_ref_change(alc, sub, v, 1, false);

                val = vgen_value_and_exec(alc, v, sub, true, true);

                ul->ancestors = NULL;
                ul->upref_token = val->item;
                ul->read_after_move = false;
            }
        }

    } else if (vt == v_ir_val) {
        IRVal *irv = val->item;
        irv->value = usage_move_value(alc, fc, scope, irv->value, storage_type);
    } else if (vt == v_class_pa) {
        VClassPA *pa = val->item;
        if (pa->deref_token) {
            TExec *exec = pa->deref_token->item;
            exec->enable = false;
        }
    } else if (vt == v_array_item) {
        VArrayItem *ai = val->item;
        if (ai->deref_token) {
            TExec *exec = ai->deref_token->item;
            exec->enable = false;
        }
    } else if (vt == v_global) {
        VGlobal *vg = val->item;
        if (vg->deref_token) {
            TExec *exec = vg->deref_token->item;
            exec->enable = false;
        }
    } else if (vt == v_cast) {
        Value *on = val->item;
        if (type_tracks_ownership(on->rett) && type_tracks_ownership(val->rett)) {
            val = value_init(alc, v_upref_value, val, val->rett);
        }
    } else if (vt == v_ref) {
        val = value_init(alc, v_upref_value, val, val->rett);
    } else if (vt == v_or_break) {
        VOrBreak *vob = val->item;
        vob->value = usage_move_value(alc, fc, scope, vob->value, storage_type);
    } else if (vt == v_or_value) {
        VOrValue *vov = val->item;
        vov->left = usage_move_value(alc, fc, scope, vov->left, storage_type);
        vov->right = usage_move_value(alc, fc, vov->value_scope, vov->right, storage_type);
    } else if (vt == v_fcall) {
        VFcall *fcall = val->item;
        UsageLine *ul = fcall->ul;
        if (ul) {
            ul->enable = false;
        }
    } else if (vt == v_class_init) {
        VClassInit *item = val->item;
        UsageLine *ul = item->ul;
        if (ul) {
            ul->enable = false;
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
            ul->ancestors = NULL;
            ul->upref_token = NULL;
            ul->deref_token = NULL;
            ul->parent = NULL;
            ul->clone_from = NULL;
            ul->deref_scope = NULL;
            ul->read_after_move = false;
            ul->scope = scope;
            ul->first_in_scope = true;

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
        new_ul->first_in_scope = false;

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

            if (!right->did_return) {
                new_ul->moves = max_num(new_ul->moves, r_ul->moves);
                new_ul->reads_after_move = max_num(new_ul->reads_after_move, r_ul->reads_after_move);
            }
            new_ul->moves_possible = max_num(new_ul->moves_possible, r_ul->moves_possible);

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

void end_usage_line(Allocator *alc, UsageLine *ul, Array *ast) {
    //
    if (!ul->enable)
        return;

    Decl *decl = ul->decl;
    Type *type = decl->type;

    if (type->borrow || type->weak_ptr || type->raw_ptr) {
        return;
    }

    Class *class = type->class;
    if (class && (class->must_deref || class->must_ref)) {

        if (ul->upref_token && !ul->read_after_move) {
            // Disable upref
            ul->upref_token->enable_exec = false;

        } else if (ul->ancestors && !ul->read_after_move) {

            //
            for (int i = 0; i < ul->ancestors->length; i++) {
                UsageLine *anc = array_get_index(ul->ancestors, i);
                if (!anc->scope->did_return) {
                    end_usage_line(alc, anc, anc->scope->ast);
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
                    if (!oldest->first_in_scope) {
                        all_deref = false;
                        break;
                    }
                    if (!oldest->deref_token) {
                        all_deref = false;
                        break;
                    }
                    if (oldest->read_after_move) {
                        read_after_move = true;
                        break;
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
                    end_usage_line(alc, ul->clone_from, ul->clone_from->scope->ast);
                }
            }

        } else {
            // Add deref token
            if (class->must_deref) {
                Scope *sub = scope_init(alc, sct_default, ul->scope, true);
                Value *val = value_init(alc, v_decl, decl, type);
                class_ref_change(alc, sub, val, -1, false);
                Token *t = tgen_exec(alc, sub, true);
                array_push((ul->deref_scope ? ul->deref_scope->ast : ul->scope->ast), t);
                ul->deref_token = t->item;
            }
        }
    }
}

void deref_expired_decls(Allocator *alc, Scope *scope, Array *ast) {
    //
    Array *decls = scope->usage_keys;
    if (decls) {
        for (int i = 0; i < decls->length; i++) {
            Decl *decl = array_get_index(decls, i);
            UsageLine *ul = array_get_index(scope->usage_values, i);

            if (decl->scope == scope) {
                end_usage_line(alc, ul, ul->scope->ast);
            }
        }
    }

    // Defer
    if (scope->defer_ast) {
        Array *def_ast = scope->defer_ast;
        for (int i = 0; i < def_ast->length; i++) {
            Token *t = array_get_index(def_ast, i);
            array_push(scope->ast, t);
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
                        end_usage_line(alc, ul, ul->scope->ast);
                    }
                    break;
                }

                if (scope == until)
                    break;
                scope = scope->parent;
            }
        }
    }

    // Defer
    Scope *scope = scope_;
    while (scope) {
        if (scope->defer_ast) {
            Array *def_ast = scope->defer_ast;
            for (int i = 0; i < def_ast->length; i++) {
                Token *t = array_get_index(def_ast, i);
                array_push(scope_->ast, t);
            }
        }
        if (scope == until)
            break;
        scope = scope->parent;
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
