
#include "../all.h"

Value *value_handle_idf(Fc *fc, Allocator *alc, Scope *scope, Id *id, Idf *idf);
void value_equalize_types(Allocator *alc, Fc *fc, Scope *scope, VPair *pair);
Value *value_func_call(Allocator *alc, Fc *fc, Scope *scope, Value *on);

Value *value_init(Allocator *alc, int type, void *item, Type *rett) {
    //
    Value *v = al(alc, sizeof(Value));
    v->type = type;
    v->item = item;
    v->rett = rett;
    v->issets = NULL;

    return v;
}

Value *read_value(Fc *fc, Allocator *alc, Scope *scope, bool sameline, int prio, bool assignable) {
    //
    char *token = fc->token;
    Build *b = fc->b;
    Value *v = NULL;

    tok(fc, token, sameline, true);

    bool skip_move = assignable;

    if (strcmp(token, "(") == 0) {
        v = read_value(fc, alc, scope, false, 0, false);
        tok_expect(fc, ")", false, true);
        skip_move = true;
        //
    } else if (strcmp(token, "\"") == 0) {
        Chunk before_str[1];
        *before_str = *fc->chunk;
        Str *str = read_string(fc);
        char *body = str_to_chars(alc, str);
        if (get_char(fc, 0) == '{') {
            // Format string
            *fc->chunk = *before_str;
            Array *parts = read_string_chunks(alc, fc);

            tok_expect(fc, "{", true, false);

            int valuec = parts->length - 1;
            Array *values = array_make(alc, valuec);
            for (int i = 0; i < valuec; i++) {
                if (i > 0) {
                    tok_expect(fc, ",", false, true);
                }
                Value *val = read_value(fc, alc, scope, false, 0, false);
                if (!type_is_string(val->rett, b)) {
                    sprintf(fc->sbuf, "Format string values must return a string");
                    fc_error(fc);
                }
                array_push(values, val);
            }
            VFString *vfs = al(alc, sizeof(VFString));
            vfs->parts = parts;
            vfs->values = values;
            vfs->line = before_str->line;
            vfs->col = before_str->col;
            Type *rett = type_gen(fc->b, alc, "String");
            rett->strict_ownership = true;
            v = value_init(alc, v_fstring, vfs, rett);

            tok_expect(fc, "}", false, true);
        } else {
            Type *rett = type_gen(fc->b, alc, "String");
            rett->strict_ownership = true;
            v = value_init(alc, v_string, body, rett);
        }
        //
    } else if (strcmp(token, "'") == 0) {
        char ch = get_char(fc, 0);
        chunk_move(fc->chunk, 1);
        if (ch == '\\') {
            char nch = get_char(fc, 0);
            chunk_move(fc->chunk, 1);
            if (nch == '0') {
                ch = '\0';
            } else if (nch == 'n') {
                ch = '\n';
            } else if (nch == 'r') {
                ch = '\r';
            } else if (nch == 't') {
                ch = '\t';
            } else if (nch == 'v') {
                ch = '\v';
            } else if (nch == 'f') {
                ch = '\f';
            } else if (nch == 'b') {
                ch = '\b';
            } else if (nch == 'a') {
                ch = '\a';
            }
        }

        tok_expect(fc, "'", true, false);

        v = vgen_vint(alc, ch, type_gen(b, alc, "u8"), true);
        //
    } else if (strcmp(token, "!") == 0) {
        Value *on = read_value(fc, alc, scope, false, 8, false);
        if (!type_is_bool(on->rett, b)) {
            sprintf(fc->sbuf, "Value after '!' must be a bool");
            fc_error(fc);
        }
        v = vgen_compare(alc, b, on, vgen_vint(alc, 0, type_gen(b, alc, "bool"), true), op_eq);
    } else if (strcmp(token, "&") == 0) {
        Value *on = read_value(fc, alc, scope, false, 8, false);
        if (on->rett->strict_ownership) {
            Type *rett = type_clone(alc, on->rett);
            rett->strict_ownership = false;
            rett->shared_ref = true;
            rett->borrow = false;
            on->rett = rett;
        }
        v = value_init(alc, v_ref, on, on->rett);
    } else if (strcmp(token, "true") == 0) {
        v = vgen_vint(alc, 1, type_gen(b, alc, "bool"), true);
    } else if (strcmp(token, "false") == 0) {
        v = vgen_vint(alc, 0, type_gen(b, alc, "bool"), true);
    } else if (strcmp(token, "null") == 0) {
        v = vgen_null(alc, b);
    } else if (strcmp(token, "@swap") == 0) {

        Value *var = read_value(fc, alc, scope, false, 0, false);
        if (!value_is_assignable(var)) {
            sprintf(fc->sbuf, "The first argument in '@swap' must be assignable. e.g. a variable");
            fc_error(fc);
        }
        if (var->type == v_decl) {
            Decl *decl = var->item;
            if (!decl->is_mut) {
                decl->is_mut = true;
            }
        }

        tok_expect(fc, "with", false, true);

        Value *with = read_value(fc, alc, scope, false, 0, false);
        with = try_convert(fc, alc, with, var->rett);
        type_check(fc, var->rett, with->rett);

        with = usage_move_value(alc, fc, scope, with, var->rett);

        v = vgen_swap(alc, var, with);

    } else if (strcmp(token, "@ptr_of") == 0) {
        tok_expect(fc, "(", true, false);
        Value *on = read_value(fc, alc, scope, false, 0, false);
        if (!value_is_assignable(on)) {
            sprintf(fc->sbuf, "Value used in '@ptr_of' must be assignable");
            fc_error(fc);
        }
        value_disable_upref_deref(on);
        tok_expect(fc, ")", false, true);
        if (on->type == v_decl) {
            Decl *decl = on->item;
            if (!decl->is_mut) {
                decl->is_mut = true;
            }
        }
        v = value_init(alc, v_getptr, on, type_gen(b, alc, "ptr"));

    } else if (strcmp(token, "@array_of") == 0) {
        tok_expect(fc, "(", true, false);
        Value *on = read_value(fc, alc, scope, false, 0, false);
        if (!value_is_assignable(on)) {
            sprintf(fc->sbuf, "Value used in '@array_of' must be assignable");
            fc_error(fc);
        }
        value_disable_upref_deref(on);
        tok_expect(fc, ")", false, true);
        if (on->type == v_decl) {
            Decl *decl = on->item;
            if (!decl->is_mut) {
                decl->is_mut = true;
            }
        }
        Type *rett = type_array_of(alc, b, on->rett, 1);
        rett->borrow = true;
        v = value_init(alc, v_getptr, on, rett);
        //

    } else if (strcmp(token, "@ptr_val") == 0) {
        tok_expect(fc, "(", true, false);
        Value *on = read_value(fc, alc, scope, false, 0, false);
        Type *rett = on->rett;
        if (rett->ptr_depth == 0 || (!rett->class && !rett->array_of)) {
            sprintf(fc->sbuf, "Value used in '@ptr_val' must be a pointer value and the sub-type must be known (so 'ptr' also does not work)");
            fc_error(fc);
        }
        tok_expect(fc, ")", false, true);
        if (rett->ptr_depth == 1 && rett->type == type_arr) {
            rett = rett->array_of;
        } else {
            rett = type_get_inline(alc, rett);
        }
        v = value_init(alc, v_ptrval, on, rett);

    } else if (strcmp(token, "@stack_alloc") == 0) {

        Scope *fscope = scope_find(scope, sct_func);
        if (!fscope) {
            sprintf(fc->sbuf, "You cannot use stack_alloc outside a function");
            fc_error(fc);
        }

        Func *func = fscope->func;
        func->uses_stack_alloc = true;

        tok_expect(fc, "(", true, false);
        Value *val = read_value(fc, alc, scope, false, 0, false);

        if (val->rett->type != type_int) {
            sprintf(fc->sbuf, "Stack alloc value must return an integer");
            fc_error(fc);
        }

        tok_expect(fc, ")", false, true);
        v = value_init(alc, v_stack_alloc, val, type_gen(b, alc, "ptr"));

    } else if (strcmp(token, "@stack_object") == 0) {

        Scope *fscope = scope_find(scope, sct_func);
        if (!fscope) {
            sprintf(fc->sbuf, "You cannot use stack_alloc outside a function");
            fc_error(fc);
        }

        Func *func = fscope->func;
        func->uses_stack_alloc = true;

        tok_expect(fc, "(", true, false);
        Type *type = read_type(fc, alc, scope, false, true, rtc_default);
        if (!type->class) {
            sprintf(fc->sbuf, "@stack_make type must have a class");
            fc_error(fc);
        }
        Class *class = type->class;

        tok_expect(fc, ")", false, true);

        Type *rett = type_gen_class(alc, class);
        rett->borrow = true;
        v = value_init(alc, v_stack_alloc, vgen_vint(alc, class->size, type_gen(b, alc, "i32"), false), rett);

    } else if (strcmp(token, "@atomic_op") == 0) {

        Value *on = read_value(fc, alc, scope, true, 0, true);
        if (!value_is_assignable(on)) {
            sprintf(fc->sbuf, "The first parameter for 'atomicop' must be an assignable value");
            fc_error(fc);
        }
        if (on->type == v_decl) {
            Decl *decl = v->item;
            if (!decl->is_mut) {
                decl->is_mut = true;
            }
        }
        if (on->rett->type != type_int) {
            sprintf(fc->sbuf, "You can only use atomic operations on integers");
            fc_error(fc);
        }

        tok(fc, token, true, true);

        int op = op_add;
        if (strcmp(token, "ADD") == 0) {
        } else if (strcmp(token, "SUB") == 0) {
            op = op_sub;
        } else if (strcmp(token, "AND") == 0) {
            op = op_bit_and;
        } else if (strcmp(token, "OR") == 0) {
            op = op_bit_or;
        } else if (strcmp(token, "XOR") == 0) {
            op = op_bit_xor;
        } else {
            sprintf(fc->sbuf, "Unknown atomic operation '%s'. Expected: ADD,SUB,AND,OR,XOR", token);
            fc_error(fc);
        }

        Value *right = read_value(fc, alc, scope, true, 0, true);
        right = try_convert(fc, alc, right, on->rett);
        type_check(fc, on->rett, right->rett);

        v = vgen_atomicop(alc, on, right, op);

    } else if (strcmp(token, "@isset") == 0) {

        tok_expect(fc, "(", true, false);
        Value *on = read_value(fc, alc, scope, false, 0, false);

        if (!on->rett->nullable) {
            sprintf(fc->sbuf, "You can only use isset on nullable values");
            fc_error(fc);
        }

        tok_expect(fc, ")", false, true);

        v = value_init(alc, v_isset, on, type_gen(b, alc, "bool"));

        if (on->type == v_decl) {
            Array *issets = array_make(alc, 4);
            v->issets = issets;
            array_push(issets, on);
        }

    } else if (strcmp(token, "@sizeof") == 0) {
        tok_expect(fc, "(", true, false);
        Type *type = read_type(fc, alc, scope, false, true, rtc_default);
        tok_expect(fc, ")", false, true);
        v = vgen_vint(alc, type->bytes, type_gen(b, alc, "i32"), false);
    } else if (strcmp(token, "@sizeof_class") == 0) {
        tok_expect(fc, "(", true, false);
        Type *type = read_type(fc, alc, scope, false, true, rtc_default);
        tok_expect(fc, ")", false, true);
        Class *class = type->class;
        if (!class) {
            sprintf(fc->sbuf, "The type has no class associated with it");
            fc_error(fc);
        }
        v = vgen_vint(alc, class->size, type_gen(b, alc, "i32"), false);
        //
    } else if (strcmp(token, "{{") == 0) {
        // value scope
        // tok_expect(fc, ":", false, true);
        // Type *rett = read_type(fc, alc, scope, false, true, rtc_func_rett);
        // if (type_is_void(rett)) {
        //     sprintf(fc->sbuf, "Value scope return type cannot be void");
        //     fc_error(fc);
        // }

        Scope *sub = scope_init(alc, sct_vscope, scope, true);
        sub->vscope = al(alc, sizeof(VScope));
        sub->vscope->rett = NULL;
        sub->vscope->lvar = NULL;
        read_ast(fc, sub, false);

        if (!sub->did_return || !sub->vscope->rett) {
            sprintf(fc->sbuf, "The value scope '{{' did not return a value");
            fc_error(fc);
        }

        v = value_init(alc, v_scope, sub, sub->vscope->rett);

    } else if (strcmp(token, "@ptrv") == 0) {
        tok_expect(fc, "(", false, true);
        // On
        Value *on = read_value(fc, alc, scope, false, 0, false);
        // Type
        tok_expect(fc, ",", false, true);
        if (on->rett->type != type_ptr) {
            sprintf(fc->sbuf, "You can only use 'ptrv' on pointer type values");
            fc_error(fc);
        }
        Type *type = read_type(fc, alc, scope, true, true, rtc_ptrv);
        // Index
        tok_expect(fc, ",", false, true);
        Value *index = read_value(fc, alc, scope, false, 0, false);
        if (index->rett->type != type_int) {
            sprintf(fc->sbuf, "@ptrv index must be of type integer");
            fc_error(fc);
        }
        tok_expect(fc, ")", false, true);

        v = vgen_ptrv(alc, on, type, index);
        //

    } else if (is_number(token[0]) || strcmp(token, "-") == 0) {

        bool is_negative = strcmp(token, "-") == 0;

        if (is_negative) {
            tok(fc, token, true, false);
        }

        bool is_float = false;
        long int iv = 0;
        float fv = 0;

        if (strcmp(token, "0") == 0 && get_char(fc, 0) == 'x') {
            //
            chunk_move(fc->chunk, 1);
            read_hex(fc, token);
            iv = hex2int(token);
        } else if (is_number(token[0])) {
            char *num_str = dups(alc, token);
            char *float_str = NULL;
            if (get_char(fc, 0) == '.') {

                if (is_number(get_char(fc, 1))) {
                    is_float = true;

                    chunk_move(fc->chunk, 1);
                    tok(fc, token, true, false);

                    float_str = al(alc, strlen(num_str) + strlen(token) + 2);
                    strcpy(float_str, num_str);
                    strcpy(float_str, ".");
                    strcpy(float_str, token);
                }
            }
            if (is_float) {
                fv = atof(float_str);
            } else {
                iv = atoi(num_str);
            }
        } else {
            sprintf(fc->sbuf, "Invalid number: '%s'", token);
            fc_error(fc);
        }

        if (is_float) {
            if (is_negative) {
                fv *= -1;
            }
            v = vgen_vfloat(alc, fc->b, fv, false);
        } else {
            if (is_negative) {
                iv *= -1;
            }
            if (get_char(fc, 0) == '#') {
                chunk_move(fc->chunk, 1);
                Type *type = read_type(fc, alc, scope, true, false, rtc_default);
                if (type->type != type_int) {
                    sprintf(fc->sbuf, "Invalid integer type");
                    fc_error(fc);
                }
                v = vgen_vint(alc, iv, type, true);
            } else {
                v = vgen_vint(alc, iv, type_gen(fc->b, alc, "i32"), false);
            }
        }

        //
    } else if (is_valid_varname_char(token[0])) {
        rtok(fc);
        Id *id = read_id(fc, sameline, true, true);
        Idf *idf = idf_by_id(fc, scope, id, true);
        v = value_handle_idf(fc, alc, scope, id, idf);
    } else {
        sprintf(fc->sbuf, "Unknown value: '%s'", token);
        fc_error(fc);
    }

    if (!v) {
        sprintf(fc->sbuf, "Missing value (bug)");
        fc_error(fc);
    }

    tok(fc, token, true, false);
    while (strcmp(token, ".") == 0 || strcmp(token, "(") == 0 || strcmp(token, "++") == 0 || strcmp(token, "--") == 0 || strcmp(token, "[") == 0) {

        Type *rett = v->rett;

        if (rett->nullable) {
            sprintf(fc->sbuf, "Cannot use '%s' on a nullable type", token);
            fc_error(fc);
        }

        if (strcmp(token, ".") == 0) {

            Class *class = rett->class;
            if (!class) {
                sprintf(fc->sbuf, "Unexpected '.'");
                fc_error(fc);
            }

            if (fc->lsp_file) {
                LspData *ld = b->lsp;
                Chunk *chunk = fc->chunk;
                // char msg[200];
                // sprintf(msg, "LINE: %d/%d COL %d/%d\n", chunk->line, ld->line, chunk->col, ld->col);
                // lsp_log(msg);
                if (chunk->line == (ld->line + 1) && chunk->col == (ld->col + 2)) {
                    Array *items = array_make(b->alc, 100);
                    Array *prop_names = class->props->keys;
                    Array *props = class->props->values;
                    for (int i = 0; i < prop_names->length; i++) {
                        ClassProp *prop = array_get_index(props, i);
                        char *name = array_get_index(prop_names, i);
                        LspCompletion *c = lsp_completion_init(alc, lsp_compl_property, name);
                        if (class->fc->nsc != fc->nsc) {
                            if (prop->act == act_private) {
                                continue;
                            }
                            if (prop->act == act_readonly) {
                                c->detail = "readonly";
                            }
                        }
                        array_push(items, c);
                    }
                    Array *func_names = class->funcs->keys;
                    for (int i = 0; i < func_names->length; i++) {
                        char *name = array_get_index(func_names, i);
                        Func *func = array_get_index(class->funcs->values, i);
                        if (func->is_static)
                            continue;
                        LspCompletion *c = lsp_completion_init(alc, lsp_compl_method, name);
                        if (class->fc->nsc != fc->nsc) {
                            if (func->act == act_private || func->is_generated) {
                                continue;
                            }
                        }
                        c->insert = lsp_func_insert(alc, func, name, true);
                        array_push(items, c);
                    }
                    lsp_completion_respond(b, ld, items);
                }
            }

            tok(fc, token, true, false);

            ClassProp *prop = map_get(class->props, token);
            if (prop) {
                // Class prop
                if (prop->act == act_private && class->fc->nsc != fc->nsc) {
                    sprintf(fc->sbuf, "Trying to access a private property from another namespace");
                    fc_error(fc);
                }

                v = vgen_class_pa(alc, scope, v, prop);
            } else {
                // Class func
                Func *func = map_get(class->funcs, token);
                if (!func) {
                    sprintf(fc->sbuf, "Property does not exist: '%s'", token);
                    fc_error(fc);
                }
                if (func->is_static) {
                    sprintf(fc->sbuf, "Trying to access static function in a non-static way: '%s'", token);
                    fc_error(fc);
                }
                v = vgen_fptr(alc, func, v);
            }

        } else if (strcmp(token, "(") == 0) {
            // // Func call
            v = value_func_call(alc, fc, scope, v);
        } else if (strcmp(token, "++") == 0 || strcmp(token, "--") == 0) {
            if (!value_is_assignable(v)) {
                sprintf(fc->sbuf, "Cannot use ++ or -- on this value (not assignable)");
                fc_error(fc);
            }
            if (v->type == v_decl) {
                Decl *decl = v->item;
                if (!decl->is_mut) {
                    decl->is_mut = true;
                }
            }
            Type *vt = v->rett;
            int vtt = vt->type;
            if (vtt != type_int && vtt != type_ptr) {
                sprintf(fc->sbuf, "Cannot use ++ or -- on this value");
                fc_error(fc);
            }
            bool is_ptr = vtt == type_ptr;
            v = vgen_incr_decr(alc, v, strcmp(token, "++") == 0);
            //
        } else if (strcmp(token, "[") == 0) {
            Type *rett = v->rett;
            if (rett->type != type_arr) {
                sprintf(fc->sbuf, "Unexpected '['");
                fc_error(fc);
            }
            Value *index = read_value(fc, alc, scope, true, 0, false);
            if (index->rett->type != type_int) {
                sprintf(fc->sbuf, "Array index expression must return an integer value");
                fc_error(fc);
            }

            tok_expect(fc, "]", true, true);

            v = vgen_array_item(alc, scope, v, index);
        }

        tok(fc, token, true, false);
    }

    rtok(fc);
    tok(fc, token, false, true);

    if (prio == 0 || prio > 7) {
        while (strcmp(token, "@as") == 0) {
            if (type_is_void(v->rett)) {
                sprintf(fc->sbuf, "Left side of '@as' must return a value");
                fc_error(fc);
            }

            Type *type = read_type(fc, alc, scope, false, true, rtc_ptrv);
            v = vgen_cast(alc, v, type);

            tok(fc, token, false, true);
        }
    }

    if (prio == 0 || prio > 8) {
        while (strcmp(token, "?") == 0) {
            if (!type_is_bool(v->rett, b)) {
                sprintf(fc->sbuf, "You can only use '?' on bool values");
                fc_error(fc);
            }

            Scope *true_scope = usage_scope_init(alc, scope, sct_default);
            Scope *false_scope = usage_scope_init(alc, scope, sct_default);
            Array *ancestors = array_make(alc, 10);
            array_push(ancestors, true_scope);
            array_push(ancestors, false_scope);

            Value *left = read_value(fc, alc, true_scope, false, 0, false);
            tok_expect(fc, ":", false, true);
            Value *right = read_value(fc, alc, false_scope, false, 0, false);

            Type *type = type_merge(b, alc, left->rett, right->rett);

            left = try_convert(fc, alc, left, type);
            right = try_convert(fc, alc, right, type);

            // type_check(fc, type, left->rett);
            // type_check(fc, type, right->rett);
            char *reason;
            if (!type_compat(type, left->rett, &reason) || !type_compat(type, right->rett, &reason)) {
                char t1s[200];
                char t2s[200];
                type_to_str(left->rett, t1s);
                type_to_str(right->rett, t2s);
                sprintf(fc->sbuf, "Types in '... ? x : y' statement do not match: '%s' <-> '%s'", t1s, t2s);
                fc_error(fc);
            }

            usage_merge_ancestors(alc, scope, ancestors);

            v = vgen_this_or_that(alc, v, true_scope, left, false_scope, right, type);

            tok(fc, token, false, true);
        }
    }

    if (prio == 0 || prio > 9) {
        while (strcmp(token, "?!") == 0 || strcmp(token, "??") == 0) {

            Type *ltype = v->rett;
            if (!ltype->nullable) {
                sprintf(fc->sbuf, "Left side will never be null");
                fc_error(fc);
            }

            Scope *deref_scope = usage_create_deref_scope(alc, scope);

            Scope *else_scope = usage_scope_init(alc, scope, sct_default);
            Scope *usage_scope = usage_scope_init(alc, scope, sct_default);
            Array *ancestors = array_make(alc, 10);
            array_push(ancestors, usage_scope);
            array_push(ancestors, else_scope);

            if (strcmp(token, "?!") == 0) {
                // ?!
                tok(fc, token, false, true);
                bool single_line = strcmp(token, "{") != 0;
                if (single_line)
                    rtok(fc);

                read_ast(fc, usage_scope, single_line);
                usage_merge_ancestors(alc, scope, ancestors);

                if (!usage_scope->did_return) {
                    sprintf(fc->sbuf, "Scope did not use return, break, continue, exit or panic");
                    fc_error(fc);
                }
                v = vgen_or_break(alc, v, usage_scope, else_scope, deref_scope);
            } else {
                // ??
                Value *right = read_value(fc, alc, usage_scope, true, 9, false);
                usage_merge_ancestors(alc, scope, ancestors);

                if (v->rett->borrow != right->rett->borrow) {
                    sprintf(fc->sbuf, "One side of '\?\?' has a borrowed value and the other does not");
                    fc_error(fc);
                }

                type_check(fc, v->rett, right->rett);

                v = vgen_or_value(alc, v, right, usage_scope, else_scope, deref_scope);
            }

            tok(fc, token, false, true);
        }
    }

    if (prio == 0 || prio > 10) {
        while (strcmp(token, "*") == 0 || strcmp(token, "/") == 0 || strcmp(token, "%") == 0) {
            int op = op_mul;
            if (strcmp(token, "/") == 0) {
                op = op_div;
            } else if (strcmp(token, "%") == 0) {
                op = op_mod;
            }

            Value *right = read_value(fc, alc, scope, false, 10, false);
            v = value_op(fc, alc, scope, v, right, op);

            tok(fc, token, false, true);
        }
    }

    if (prio == 0 || prio > 20) {
        while (strcmp(token, "+") == 0 || strcmp(token, "-") == 0) {
            int op = op_add;
            if (strcmp(token, "-") == 0) {
                op = op_sub;
            }

            Value *right = read_value(fc, alc, scope, false, 20, false);
            v = value_op(fc, alc, scope, v, right, op);

            tok(fc, token, false, true);
        }
    }

    if (prio == 0 || prio > 25) {
        while (strcmp(token, "<<") == 0 || strcmp(token, ">>") == 0) {
            int op = op_shl;
            if (strcmp(token, ">>") == 0) {
                op = op_shr;
            }

            Value *right = read_value(fc, alc, scope, false, 25, false);
            v = value_op(fc, alc, scope, v, right, op);

            tok(fc, token, false, true);
        }
    }

    if (prio == 0 || prio > 30) {
        sprintf(fc->sbuf, ".%s.", token);
        while (strstr(".==.!=.<=.>=.<.>.", fc->sbuf)) {
            int op = op_eq;
            if (strcmp(token, "!=") == 0) {
                op = op_ne;
            } else if (strcmp(token, "<") == 0) {
                op = op_lt;
            } else if (strcmp(token, "<=") == 0) {
                op = op_lte;
            } else if (strcmp(token, ">") == 0) {
                op = op_gt;
            } else if (strcmp(token, ">=") == 0) {
                op = op_gte;
            }

            int line = fc->chunk->line;
            int col = fc->chunk->col;

            Value *right = read_value(fc, alc, scope, false, 30, false);

            bool magic = false;

            Class *lclass = v->rett->class;
            if (lclass) {
                if (op == op_eq || op == op_ne) {
                    Func *func = map_get(lclass->funcs, "__eq");
                    if (func) {
                        Array *values = array_make(alc, 4);
                        array_push(values, v);
                        array_push(values, right);
                        Value *on = vgen_fptr(alc, func, NULL);
                        fcall_type_check(fc, on, values);
                        v = vgen_fcall(alc, scope, on, values, func->rett, NULL, line, col);
                        if (op == op_ne) {
                            v = vgen_compare(alc, b, v, vgen_vint(alc, 0, type_gen(b, alc, "bool"), true), op_eq);
                        }
                        magic = true;
                    }
                } else if (op == op_lt) {
                    Func *func = map_get(lclass->funcs, "__lt");
                    if (func) {
                        Array *values = array_make(alc, 4);
                        array_push(values, v);
                        array_push(values, right);
                        Value *on = vgen_fptr(alc, func, NULL);
                        fcall_type_check(fc, on, values);
                        v = vgen_fcall(alc, scope, on, values, func->rett, NULL, line, col);
                        magic = true;
                    }
                } else if (op == op_gt) {
                    Func *func = map_get(lclass->funcs, "__gt");
                    if (func) {
                        Array *values = array_make(alc, 4);
                        array_push(values, v);
                        array_push(values, right);
                        Value *on = vgen_fptr(alc, func, NULL);
                        fcall_type_check(fc, on, values);
                        v = vgen_fcall(alc, scope, on, values, func->rett, NULL, line, col);
                        magic = true;
                    }
                }
            }

            if (!magic) {
                VPair *pair = al(alc, sizeof(VPair));
                pair->left = v;
                pair->right = right;
                value_equalize_types(alc, fc, scope, pair);
                Value *left = pair->left;
                Value *right = pair->right;
                type_check(fc, left->rett, right->rett);
                v = vgen_compare(alc, b, left, right, op);
            }

            tok(fc, token, false, true);
            sprintf(fc->sbuf, ".%s.", token);
        }
    }

    if (prio == 0 || prio > 35) {
        while (strcmp(token, "&") == 0 || strcmp(token, "|") == 0 || strcmp(token, "^") == 0) {
            int op = op_bit_and;
            if (strcmp(token, "|") == 0) {
                op = op_bit_or;
            } else if (strcmp(token, "^") == 0) {
                op = op_bit_xor;
            }

            Value *right = read_value(fc, alc, scope, false, 35, false);
            v = value_op(fc, alc, scope, v, right, op);

            tok(fc, token, false, true);
        }
    }

    if (prio == 0 || prio > 40) {
        Class *bool_class = ki_get_class(b, "type", "bool");
        while (strcmp(token, "&&") == 0 || strcmp(token, "||") == 0) {

            if (v->rett->class != bool_class) {
                sprintf(fc->sbuf, "Left side must return a bool");
                fc_error(fc);
            }

            int op = op_and;
            if (strcmp(token, "||") == 0) {
                op = op_or;
            }

            Value *right = read_value(fc, alc, scope, false, 40, false);

            if (right->rett->class != bool_class) {
                sprintf(fc->sbuf, "Right side must return a bool");
                fc_error(fc);
            }

            v = vgen_and_or(alc, b, v, right, op);

            tok(fc, token, false, true);
        }
    }

    rtok(fc);

    return v;
}

Value *value_handle_idf(Fc *fc, Allocator *alc, Scope *scope, Id *id, Idf *idf) {
    //
    char *token = fc->token;

    if (idf->type == idf_decl_type_overwrite) {
        DeclOverwrite *dov = idf->item;
        Decl *decl = dov->decl;
        UsageLine *ul = usage_line_get(scope, decl);
        if (ul)
            ul->read_after_move = true;
        return value_init(alc, v_decl, decl, dov->type);
    }
    if (idf->type == idf_decl) {
        Decl *decl = idf->item;
        UsageLine *ul = usage_line_get(scope, decl);
        if (ul) {
            if (ul->moves > 0) {
                sprintf(fc->sbuf, "Read after move, variable: '%s'", decl->name);
                fc_error(fc);
            }
            ul->read_after_move = true;
        }
        return value_init(alc, v_decl, decl, decl->type);
    }

    if (idf->type == idf_global) {
        Global *g = idf->item;
        VGlobal *vg = al(alc, sizeof(VGlobal));
        vg->g = g;
        vg->llvm_val = NULL;
        vg->deref_token = NULL;
        vg->upref_token = NULL;

        Type *type = g->type;
        Value *res = value_init(alc, v_global, vg, type);

        if (scope && type_tracks_ownership(type)) {
            Type *rett = type_clone(alc, type);
            rett->shared_ref = true;
            res->rett = rett;

            Value *from = vgen_ir_from(alc, res);
            vg->deref_token = tgen_ref_change_exec(alc, scope, from, -1);
            vg->upref_token = tgen_ref_change_exec(alc, scope, from, 1);
            scope_add_defer_token(alc, scope, vg->deref_token);
        }
        return res;
    }

    if (idf->type == idf_func) {
        Func *func = idf->item;
        return vgen_fptr(alc, func, NULL);
    }

    if (idf->type == idf_class) {
        Class *class = idf->item;

        if (class->is_generic_base) {
            Array *generic_types = read_generic_types(fc, scope, class);
            class = class_get_generic_class(class, generic_types);
        }

        if (get_char(fc, 0) == '.') {
            chunk_move(fc->chunk, 1);
            // Static func

            if (fc->lsp_file) {
                Build *b = fc->b;
                LspData *ld = b->lsp;
                Chunk *chunk = fc->chunk;
                if (chunk->line == (ld->line + 1) && chunk->col == (ld->col + 2)) {
                    Array *items = array_make(b->alc, 100);
                    Array *funcs = class->funcs->values;
                    Array *func_names = class->funcs->keys;
                    for (int i = 0; i < func_names->length; i++) {
                        char *name = array_get_index(func_names, i);
                        Func *func = array_get_index(funcs, i);
                        if (!func->is_static)
                            continue;
                        LspCompletion *c = lsp_completion_init(alc, lsp_compl_method, name);
                        c->insert = lsp_func_insert(alc, func, name, false);
                        array_push(items, c);
                    }
                    lsp_completion_respond(b, ld, items);
                }
            }

            tok(fc, token, true, false);
            Func *func = map_get(class->funcs, token);
            if (!func || !func->is_static) {
                sprintf(fc->sbuf, "Unknown static function: '%s'", token);
                fc_error(fc);
            }
            // Property access
            // if (cfunc->act_type == act_private) {
            //     if (scope_is_subscope_of(scope, class->scope) == false) {
            //         fc_error(fc, "Accessing a private function outside the class", NULL);
            //     }
            // }

            //
            return vgen_fptr(alc, func, NULL);
        }

        tok(fc, token, true, true);
        if (strcmp(token, "{") == 0) {
            // Class init
            Map *values = map_make(alc);
            tok(fc, token, false, true);
            while (strcmp(token, "}") != 0) {
                ClassProp *prop = map_get(class->props, token);
                if (!prop) {
                    sprintf(fc->sbuf, "Unknown property: '%s'", token);
                    fc_error(fc);
                }
                // TODO acct check

                char name[KI_TOKEN_MAX];
                strcpy(name, token);

                tok_expect(fc, ":", false, true);

                Value *value = read_value(fc, alc, scope, true, 0, false);
                value = try_convert(fc, alc, value, prop->type);
                type_check(fc, prop->type, value->rett);

                value = usage_move_value(alc, fc, scope, value, prop->type);

                map_set(values, name, value);
                //
                tok(fc, token, false, true);
                if (strcmp(token, ",") == 0) {
                    tok(fc, token, false, true);
                }
            }
            for (int i = 0; i < class->props->keys->length; i++) {
                char *key = array_get_index(class->props->keys, i);
                ClassProp *prop = array_get_index(class->props->values, i);
                Value *v = map_get(values, key);
                if (v)
                    continue;
                if (!prop->value) {
                    sprintf(fc->sbuf, "Missing property: '%s'", key);
                    fc_error(fc);
                }
                map_set(values, key, prop->value);
            }
            return vgen_class_init(alc, scope, class, values);
        }

        sprintf(fc->sbuf, "Unexpected token '%s'", token);
        fc_error(fc);
    }

    if (idf->type == idf_enum) {
        Enum *enu = idf->item;
        tok_expect(fc, ".", true, false);

        tok(fc, token, true, false);
        if (!map_contains(enu->values, token)) {
            sprintf(fc->sbuf, "Enum property does not exist '%s'", token);
            fc_error(fc);
        }
        int value = (int)(intptr_t)map_get(enu->values, token);

        return vgen_vint(alc, value, type_gen(fc->b, alc, "i32"), false);
    }

    if (idf->type == idf_fc) {
        Fc *rfc = idf->item;

        tok_expect(fc, ".", true, false);
        tok(fc, token, true, false);

        Idf *idf_ = idf_get_from_header(rfc, token, 0);
        if (!idf_) {
            sprintf(fc->sbuf, "Unknown property: '%s'", token);
            fc_error(fc);
        }

        return value_handle_idf(fc, alc, scope, id, idf_);
    }

    if (idf->type == idf_err_code) {
        Decl *decl = idf->item;
        if (get_char(fc, 0) == '#') {
            chunk_move(fc->chunk, 1);
            tok(fc, token, true, false);
            Array *errors = decl->type->func_errors;
            int index = array_find(errors, token, arr_find_str);
            if (index < 0) {
                sprintf(fc->sbuf, "The function has no error named '%s'", token);
                fc_error(fc);
            }
            return vgen_vint(alc, index + 1, type_gen(fc->b, alc, "i32"), false);
        }
        return value_init(alc, v_decl, decl, decl->type);
    }

    if (idf->type == idf_macro) {
        Macro *mac = idf->item;

        Map *values = map_make(alc);
        Array *repeat_values = array_make(alc, 8);
        Array *repeat_values_empty = array_make(alc, 8);
        array_push(repeat_values_empty, NULL);

        Array *groups = mac->groups;
        Array *parts = mac->parts;
        Map *vars = mac->vars;
        Array *input_names = vars->keys;
        char *repeat_name = NULL;

        for (int i = 0; i < groups->length; i++) {
            MacroVarGroup *mvg = array_get_index(groups, i);
            tok_expect(fc, mvg->start, true, false);

            bool does_repeat = mvg->repeat_last_input;
            int input_count = mvg->vars->length - (does_repeat ? 1 : 0);
            if (does_repeat) {
                repeat_name = array_get_index(input_names, input_names->length - 1);
            }

            int count = 0;
            tok(fc, token, false, true);
            if (strcmp(token, mvg->end) == 0) {
            } else {
                rtok(fc);
                while (true) {
                    skip_whitespace(fc);

                    int v_start = fc->chunk->i;
                    Chunk *v_startc = fc->chunk;
                    skip_macro_input(fc, mvg->end);
                    int v_end = fc->chunk->i;
                    Chunk *v_endc = fc->chunk;

                    if (v_startc != v_endc) {
                        sprintf(fc->sbuf, "Invalid macro input (mixed macros)");
                        fc_error(fc);
                    }

                    char *value = read_part(alc, fc, v_start, v_end - v_start);
                    if (count >= input_count) {
                        array_push(repeat_values, value);
                    } else {
                        char *name = array_get_index(input_names, count);
                        map_set(values, name, value);
                    }
                    count++;

                    tok(fc, token, false, true);
                    if (strcmp(token, mvg->end) == 0) {
                        break;
                    } else if (strcmp(token, ",") == 0) {
                        continue;
                    } else {
                        sprintf(fc->sbuf, "Expected ',' or '%s', found: '%s'", mvg->end, token);
                        fc_error(fc);
                    }
                }
            }

            if (count < input_count) {
                sprintf(fc->sbuf, "Missing macro inputs. Expected%s '%d', found '%d'", does_repeat ? " a minimum of" : "", input_count, count);
                fc_error(fc);
            }
            if (!does_repeat && count > input_count) {
                sprintf(fc->sbuf, "Too many macro inputs. Expected '%d', found '%d'", input_count, count);
                fc_error(fc);
            }
        }

        Str *buf = fc->str_buf;
        str_clear(buf);
        for (int i = 0; i < parts->length; i++) {
            //
            MacroPart *part = array_get_index(parts, i);
            Array *sub_parts = part->sub_parts;

            Array *loop_values = part->loop ? repeat_values : repeat_values_empty;
            //
            for (int o = 0; o < loop_values->length; o++) {
                char *repeat_value = array_get_index(loop_values, o);
                for (int u = 0; u < sub_parts->length; u++) {
                    char *spart = array_get_index(sub_parts, u);
                    if (u % 2 == 0) {
                        // String part
                        str_append_chars(buf, spart);
                    } else {
                        // Input
                        if (repeat_value && strcmp(spart, repeat_name) == 0) {
                            char *input = repeat_value;
                            MacroVar *mv = map_get(vars, repeat_name);
                            for (int x = 0; x < mv->replaces->length; x++) {
                                MacroReplace *rep = array_get_index(mv->replaces, x);
                                input = str_replace(alc, input, rep->find, rep->with);
                            }
                            str_append_chars(buf, input);
                            continue;
                        }
                        MacroVar *mv = map_get(vars, spart);
                        char *input = map_get(values, spart);
                        if (!input || !mv) {
                            sprintf(fc->sbuf, "Cannot find macro input by name: '%s' (compiler bug)", spart);
                            fc_error(fc);
                        }
                        for (int x = 0; x < mv->replaces->length; x++) {
                            MacroReplace *rep = array_get_index(mv->replaces, x);
                            input = str_replace(alc, input, rep->find, rep->with);
                        }
                        str_append_chars(buf, input);
                    }
                }
            }
        }

        str_append_char(buf, ' ');
        char *content = str_to_chars(alc, buf);

        Chunk *chunk = chunk_init(alc, NULL);
        chunk->parent = fc->chunk;
        chunk->content = content;
        chunk->length = buf->length;

        // printf(">>>%s<<<", content);
        fc->chunk = chunk;

        return read_value(fc, alc, scope, false, 0, false);
    }

    sprintf(fc->sbuf, "Cannot convert identifier to a value: '%s'", id->name);
    fc_error(fc);
    return NULL;
}

Value *value_op(Fc *fc, Allocator *alc, Scope *scope, Value *left, Value *right, int op) {

    int line = fc->chunk->line;
    int col = fc->chunk->col;

    if (left->type == v_vint && right->type == v_vint) {
        VInt *lint = left->item;
        VInt *rint = right->item;
        if (lint->force_type == false && rint->force_type == false) {
            // If both are number literals
            if (op == op_add) {
                lint->value += rint->value;
                return left;
            } else if (op == op_sub) {
                lint->value -= rint->value;
                return left;
            } else if (op == op_mul) {
                lint->value *= rint->value;
                return left;
            } else if (op == op_div) {
                lint->value /= rint->value;
                return left;
            } else if (op == op_mod) {
                lint->value %= rint->value;
                return left;
            }
        }
    }

    if (op == op_add) {
        Class *lclass = left->rett->class;
        if (lclass) {
            Func *func = map_get(lclass->funcs, "__add");
            if (func) {
                Array *values = array_make(alc, 4);
                array_push(values, left);
                array_push(values, right);
                Value *on = vgen_fptr(alc, func, NULL);
                fcall_type_check(fc, on, values);
                return vgen_fcall(alc, scope, on, values, func->rett, NULL, line, col);
            }
        }
    }

    Type *lt = left->rett;
    Type *rt = right->rett;

    if ((lt->type == type_ptr && !lt->class->allow_math) || (rt->type == type_ptr && !rt->class->allow_math)) {
        sprintf(fc->sbuf, "Cannot use math operators on these values");
        fc_error(fc);
    }
    if (lt->type == type_void || rt->type == type_void) {
        sprintf(fc->sbuf, "Cannot use operator on void values");
        fc_error(fc);
    }
    if (lt->type == type_struct || rt->type == type_struct) {
        sprintf(fc->sbuf, "Cannot use operator on class instances");
        fc_error(fc);
    }
    if (lt->type == type_func_ptr || rt->type == type_func_ptr) {
        sprintf(fc->sbuf, "Cannot use operator on function reference values");
        fc_error(fc);
    }
    if (lt->type == type_null || rt->type == type_null) {
        sprintf(fc->sbuf, "Cannot use operator on function reference values");
        fc_error(fc);
    }
    if (lt->nullable || rt->nullable) {
        sprintf(fc->sbuf, "Cannot use operator on null-able values");
        fc_error(fc);
    }

    bool is_ptr = lt->type == type_ptr || rt->type == type_ptr;

    VPair *pair = malloc(sizeof(VPair));
    pair->left = left;
    pair->right = right;

    value_equalize_types(alc, fc, scope, pair);
    Value *l = pair->left;
    Value *r = pair->right;
    free(pair);

    type_check(fc, l->rett, r->rett);
    Value *v = vgen_op(alc, fc->b, l, r, op, is_ptr);

    return v;
}

void value_equalize_types(Allocator *alc, Fc *fc, Scope *scope, VPair *pair) {
    //
    Build *b = fc->b;
    Value *left = pair->left;
    Value *right = pair->right;
    Type *lt = left->rett;
    Type *rt = right->rett;

    // structs -> ptr
    // ptr -> uxx
    // numbers to biggest size

    if (lt->type == type_struct) {
        left = vgen_cast(alc, left, type_gen(b, alc, "ptr"));
        pair->left = left;
        lt = left->rett;
    }
    if (rt->type == type_struct) {
        right = vgen_cast(alc, right, type_gen(b, alc, "ptr"));
        pair->right = right;
        rt = right->rett;
    }
    if (lt->type == type_ptr) {
        left = vgen_cast(alc, left, type_gen(b, alc, "uxx"));
        pair->left = left;
        lt = left->rett;
    }
    if (rt->type == type_ptr) {
        right = vgen_cast(alc, right, type_gen(b, alc, "uxx"));
        pair->right = right;
        rt = right->rett;
    }
    if (lt->type == type_arr) {
        left = vgen_cast(alc, left, type_gen(b, alc, "uxx"));
        pair->left = left;
        lt = left->rett;
    }
    if (rt->type == type_arr) {
        right = vgen_cast(alc, right, type_gen(b, alc, "uxx"));
        pair->right = right;
        rt = right->rett;
    }
    if (lt->type == type_null) {
        left = vgen_vint(alc, 0, type_gen(b, alc, "u8"), false);
        pair->left = left;
        lt = left->rett;
    }
    if (rt->type == type_null) {
        right = vgen_vint(alc, 0, type_gen(b, alc, "u8"), false);
        pair->right = right;
        rt = right->rett;
    }

    if (lt->type != type_int || rt->type != type_int) {
        die("Could not convert value to a number\n");
        return;
    }

    if (left->type == v_vint && right->type == v_vint) {
        VInt *lint = left->item;
        VInt *rint = right->item;
        if (lint->force_type == false && rint->force_type == false) {
            // If both are number literals, dont convert them
            return;
        }
    }

    if (left->type == v_vint) {
        VInt *vint = left->item;
        if (vint->force_type == false) {
            left = try_convert(fc, alc, left, right->rett);
            pair->left = left;
            lt = left->rett;
            return;
        }
    }

    if (right->type == v_vint) {
        VInt *vint = right->item;
        if (vint->force_type == false) {
            right = try_convert(fc, alc, right, left->rett);
            pair->right = right;
            rt = right->rett;
            return;
        }
    }

    int bytes = lt->bytes;
    bool is_signed = lt->is_signed || rt->is_signed;
    if (rt->bytes > bytes) {
        bytes = rt->bytes;
    }

    if (lt->bytes < bytes) {
        left = vgen_cast(alc, left, type_gen_int(b, alc, bytes, is_signed));
        pair->left = left;
        lt = left->rett;
    }
    if (rt->bytes < bytes) {
        right = vgen_cast(alc, right, type_gen_int(b, alc, bytes, is_signed));
        pair->right = right;
        rt = right->rett;
    }
}

Value *try_convert(Fc *fc, Allocator *alc, Value *val, Type *to_type) {
    //
    // Class* str_class = fc.b.get_ki_class("String");
    // if totype
    //     .class == str_class {
    //         let this_class = this.rett.class;
    //         verify this_class {
    //             if this_class
    //                 != str_class {
    //                     let cfunc = this_class.funcs.get("__string") or value null;
    //                     verify cfunc {
    //                         let func = cfunc.func;
    //                         let values = Array<Value>.make(2);
    //                         values.push(this);
    //                         let on = value_gen_func_ptr(func, null);
    //                         fcall_type_check(fc, on, values);
    //                         return value_gen_fcall(fc.b, scope, on, values, func.return_type);
    //                     }
    //                 }
    //         }
    //     }

    Type *vt = val->rett;
    if (val->type == v_vint) {
        VInt *vint = val->item;
        if (to_type->type == type_int || to_type->type == type_float) {
            // vint -> vint|vfloat
            if (!vint->force_type && (vt->bytes != to_type->bytes || vt->is_signed != to_type->is_signed)) {
                if (to_type->type == type_float) {
                    return vgen_vfloat(alc, fc->b, (float)vint->value, false);
                }
                int bytes = to_type->bytes;
                int bits = bytes * 8;
                // Doesnt work for some reason
                // if bytes >= fc.b.ptr_size {
                //    bits = fc.b.ptr_size * 8 - 1;
                //}
                if (bytes >= 4) {
                    bits = 4 * 8 - 1;
                }
                long int max = INT_MAX;
                long int min = 0;
                if (to_type->is_signed) {
                    min = max * -1;
                }
                if (vint->value >= min && vint->value <= max) {
                    val->rett = to_type;
                    // } else {
                    //     print(f "{} || {} - {} ({})\n" {vint.value.str(), min.str(), max.str(), bits});
                }
            }
        }
        return val;
    }

    if (vt->type == type_int || vt->type == type_float) {
        if (to_type->type == type_int || to_type->type == type_float) {
            if (to_type->type == type_float) {
                // Float -> {int/float}
                // TODO
            } else {
                // Int -> {int/float}
                if (vt->bytes < to_type->bytes) {
                    return vgen_cast(alc, val, to_type);
                }
            }
        }

        return val;
    }

    if (vt->type == type_null) {
        if (to_type->nullable) {
            // Value* v = vgen_cast(alc, val, to_type);
            // return v;
        }
    }

    return val;
}

Value *value_func_call(Allocator *alc, Fc *fc, Scope *scope, Value *on) {
    //
    char *token = fc->token;
    Type *ont = on->rett;
    int ontt = ont->type;

    int line = fc->chunk->line;
    int col = fc->chunk->col;

    if (ontt != type_func_ptr) {
        sprintf(fc->sbuf, "Function call on non-function value");
        fc_error(fc);
    }

    Array *args = ont->func_args;
    Type *rett = ont->func_rett;
    Array *errors = ont->func_errors;
    bool can_error = ont->func_can_error;

    if (!args || !rett) {
        sprintf(fc->sbuf, "Function pointer value is missing function type information (compiler bug) | args: %p | return-type: %p", args, rett);
        fc_error(fc);
    }

    int argc = args->length;
    int index = 0;
    Array *values = array_make(alc, 4);

    Value *first_arg = NULL;
    bool upref = true;
    if (on->type == v_fptr) {
        VFuncPtr *fp = on->item;
        Func *func = fp->func;
        Value *first_val = fp->first_arg;

        if (first_val) {
            if (index == argc) {
                sprintf(fc->sbuf, "Too many arguments");
                fc_error(fc);
            }

            Arg *arg = array_get_index(func->args, 0);
            first_val = usage_move_value(alc, fc, scope, first_val, arg->type);

            type_check(fc, arg->type, first_val->rett);

            array_push(values, first_val);
            index++;
        }
    }

    tok(fc, token, false, true);
    bool named_args = strcmp(token, "{") == 0;

    if (named_args) {
        sprintf(fc->sbuf, "Named arguments: TODO");
        fc_error(fc);
    } else {
        if (strcmp(token, ")") != 0) {
            rtok(fc);
            while (true) {
                if (index == argc) {
                    sprintf(fc->sbuf, "Too many arguments");
                    fc_error(fc);
                }
                Arg *arg = array_get_index(args, index);
                index++;

                Value *val = read_value(fc, alc, scope, false, 0, false);
                val = try_convert(fc, alc, val, arg->type);
                val = usage_move_value(alc, fc, scope, val, arg->type);

                type_check(fc, arg->type, val->rett);
                array_push(values, val);

                tok(fc, token, false, true);
                if (strcmp(token, ",") == 0) {
                    continue;
                }
                if (strcmp(token, ")") != 0) {
                    sprintf(fc->sbuf, "Expected ',' or ')' instead of '%s'", token);
                    fc_error(fc);
                }
                break;
            }
        }
        // Check defaults
        while (index < argc) {
            Arg *arg = array_get_index(args, index);

            Value *val = arg->value;
            if (!val)
                break;

            array_push(values, val);
            index++;
        }
        if (index < argc) {
            sprintf(fc->sbuf, "Missing arguments");
            fc_error(fc);
        }
    }

    FCallOr * or = NULL;

    if (can_error) {

        tok(fc, token, false, true);
        if (strcmp(token, "!") == 0) {
            // !
            if (!type_is_void(rett)) {
                sprintf(fc->sbuf, "You cannot use '!' when the function returns a value. It needs an alternative value '!?' or exit the current scope '!!'");
                fc_error(fc);
            }
        } else {

            Scope *deref_scope = usage_create_deref_scope(alc, scope);
            Scope *else_scope = usage_scope_init(alc, scope, sct_default);
            Scope *usage_scope = usage_scope_init(alc, scope, sct_default);
            Array *ancestors = array_make(alc, 10);
            array_push(ancestors, usage_scope);
            array_push(ancestors, else_scope);

            or = al(alc, sizeof(FCallOr));
            or->scope = usage_scope;
            or->else_scope = else_scope;
            or->deref_scope = deref_scope;
            or->value = NULL;
            or->err_code_decl = NULL;
            or->err_msg_decl = NULL;

            if (strcmp(token, "!!") == 0) {
                // !!
                tok(fc, token, false, true);

                if (strcmp(token, "|") == 0) {
                    tok(fc, token, true, true);
                    if (!is_valid_varname(token)) {
                        sprintf(fc->sbuf, "Invalid variable name '%s'", token);
                        fc_error(fc);
                    }
                    char *err_name = dups(alc, token);
                    char *msg_name = NULL;

                    // tok_expect(fc, "|", true, true);
                    // tok(fc, token, false, true);
                    tok(fc, token, false, true);
                    if (strcmp(token, ",") == 0) {
                        tok(fc, token, true, true);
                        if (!is_valid_varname(token)) {
                            sprintf(fc->sbuf, "Invalid variable name '%s'", token);
                            fc_error(fc);
                        }
                        msg_name = dups(alc, token);
                        tok_expect(fc, "|", true, true);
                        tok(fc, token, false, true);
                    } else if (strcmp(token, "|") == 0) {
                        tok(fc, token, false, true);
                    } else {
                        sprintf(fc->sbuf, "Expected '|' or ',' but found: '%s'", token);
                        fc_error(fc);
                    }

                    Type *code_type = type_gen(fc->b, alc, "i32");
                    code_type->func_errors = errors;
                    Decl *code_decl = decl_init(alc, usage_scope, err_name, code_type, NULL, false);
                    or->err_code_decl = code_decl;
                    Idf *idf = idf_init(alc, idf_err_code);
                    idf->item = code_decl;
                    map_set(usage_scope->identifiers, err_name, idf);

                    if (msg_name) {
                        Type *msg_type = type_gen(fc->b, alc, "String");
                        Decl *msg_decl = decl_init(alc, usage_scope, msg_name, msg_type, NULL, false);
                        or->err_msg_decl = msg_decl;
                        Idf *idf = idf_init(alc, idf_decl);
                        idf->item = msg_decl;
                        map_set(usage_scope->identifiers, msg_name, idf);
                    }
                }

                bool single_line = strcmp(token, "{") != 0;
                if (single_line)
                    rtok(fc);

                read_ast(fc, usage_scope, single_line);
                usage_merge_ancestors(alc, scope, ancestors);

                if (!usage_scope->did_return) {
                    sprintf(fc->sbuf, "Scope did not use return, break, continue, exit or panic");
                    fc_error(fc);
                }

            } else if (strcmp(token, "!?") == 0) {
                // !?
                Value *right = read_value(fc, alc, usage_scope, true, 0, false);
                right = try_convert(fc, alc, right, rett);
                usage_merge_ancestors(alc, scope, ancestors);

                Type *rt = right->rett;
                if (rt->nullable && !rett->nullable && rett->ptr_depth > 0) {
                    rett = type_clone(alc, rett);
                    rett->nullable = true;
                }
                type_check(fc, rett, rt);

                or->value = right;

            } else {
                sprintf(fc->sbuf, "The function can return errors, expected '!?', '!!' or '!', but found '%s'", token);
                fc_error(fc);
            }
        }
    }

    if (on->type == v_fptr) {
        VFuncPtr *fp = on->item;
        Func *func = fp->func;
        if (func->will_exit) {
            scope->did_return = true;
            scope->did_exit_function = true;
        }
    }

    return vgen_fcall(alc, scope, on, values, rett, or, line, col);
}

bool value_is_assignable(Value *v) {
    //
    return (v->type == v_decl || v->type == v_class_pa || v->type == v_ptrv || v->type == v_global || v->type == v_array_item);
}

void value_disable_upref_deref(Value *val) {
    //
    if (val->type == v_class_pa) {
        VClassPA *pa = val->item;
        if (pa->deref_token) {
            TExec *exec = pa->deref_token->item;
            exec->enable = false;
            exec = pa->upref_token->item;
            exec->enable = false;
        }
    }
    if (val->type == v_array_item) {
        VArrayItem *ai = val->item;
        if (ai->deref_token) {
            TExec *exec = ai->deref_token->item;
            exec->enable = false;
            exec = ai->upref_token->item;
            exec->enable = false;
        }
    }
    if (val->type == v_global) {
        VGlobal *vg = val->item;
        if (vg->deref_token) {
            TExec *exec = vg->deref_token->item;
            exec->enable = false;
            exec = vg->upref_token->item;
            exec->enable = false;
        }
    }
}