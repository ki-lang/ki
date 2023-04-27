
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
    } else if (strcmp(token, "+") == 0) {
        Value *on = read_value(fc, alc, scope, false, 8, false);
        if (on->type != v_fcall && on->type != v_class_init) {
            sprintf(fc->sbuf, "You can only convert values to shared ownership if the value is a function call or class initialization");
            fc_error(fc);
        }
        if (on->rett->strict_ownership) {
            Type *rett = type_clone(alc, on->rett);
            rett->strict_ownership = false;
            on->rett = rett;
        }
        v = on;
    } else if (strcmp(token, "true") == 0) {
        v = vgen_vint(alc, 1, type_gen(b, alc, "bool"), true);
    } else if (strcmp(token, "false") == 0) {
        v = vgen_vint(alc, 0, type_gen(b, alc, "bool"), true);
    } else if (strcmp(token, "null") == 0) {
        v = vgen_null(alc, b);
    } else if (strcmp(token, "getptr") == 0) {
        Value *on = read_value(fc, alc, scope, false, 0, false);
        if (on->type != v_class_pa && on->type != v_ptrv && on->type != v_decl && on->type != v_array_item) {
            sprintf(fc->sbuf, "Value used in 'getptr' must be assignable");
            fc_error(fc);
        }
        if (on->type == v_decl) {
            Decl *decl = v->item;
            if (!decl->is_mut) {
                decl->is_mut = true;
            }
        }
        v = value_init(alc, v_getptr, on, type_gen(b, alc, "ptr"));
        //
    } else if (strcmp(token, "stack_alloc") == 0) {

        tok_expect(fc, "(", true, false);
        Value *val = read_value(fc, alc, scope, false, 0, false);

        if (val->rett->type != type_int) {
            sprintf(fc->sbuf, "Stack alloc value must return an integer");
            fc_error(fc);
        }

        tok_expect(fc, ")", false, true);
        v = value_init(alc, v_stack_alloc, val, type_gen(b, alc, "ptr"));

    } else if (strcmp(token, "atomicop") == 0) {

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

    } else if (strcmp(token, "isset") == 0) {

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

    } else if (strcmp(token, "sizeof") == 0) {
        tok_expect(fc, "(", true, false);
        Type *type = read_type(fc, alc, scope, false, true, rtc_default);
        tok_expect(fc, ")", false, true);
        v = vgen_vint(alc, type->bytes, type_gen(b, alc, "i32"), false);
    } else if (strcmp(token, "sizeof_class") == 0) {
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
    } else if (strcmp(token, "@vs") == 0) {
        // value scope
        tok_expect(fc, ":", false, true);
        Type *rett = read_type(fc, alc, scope, false, true, rtc_default);
        if (type_is_void(rett)) {
            sprintf(fc->sbuf, "Value scope return type cannot be void");
            fc_error(fc);
        }

        tok_expect(fc, "{", false, true);
        Scope *sub = scope_init(alc, sct_vscope, scope, true);
        sub->vscope = al(alc, sizeof(VScope));
        sub->vscope->rett = rett;
        sub->vscope->lvar = NULL;
        read_ast(fc, sub, false);

        if (!sub->did_return) {
            sprintf(fc->sbuf, "Value scope did not return a value");
            fc_error(fc);
        }

        v = value_init(alc, v_scope, sub, rett);

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
        sprintf(fc->sbuf, "Unknown value: '%s' | %d", token, prio);
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

            tok(fc, token, true, false);

            ClassProp *prop = map_get(class->props, token);
            if (prop) {
                // Class prop
                if (prop->act == act_private && !scope_contains(class->scope, scope)) {
                    sprintf(fc->sbuf, "Trying to access private property outside the class");
                    fc_error(fc);
                }

                v = vgen_class_pa(alc, v, prop);
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

            v = vgen_array_item(alc, v, index);
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

            Value *right = read_value(fc, alc, scope, false, 30, false);

            bool magic = false;

            if (op == op_eq || op == op_ne) {
                Class *lclass = v->rett->class;
                if (lclass) {
                    Func *func = map_get(lclass->funcs, "__eq");
                    if (func) {
                        Array *values = array_make(alc, 4);
                        array_push(values, v);
                        array_push(values, right);
                        Value *on = vgen_fptr(alc, func, NULL);
                        fcall_type_check(fc, on, values);
                        v = vgen_fcall(alc, scope, on, values, func->rett, NULL);
                        if (op == op_ne) {
                            v = vgen_compare(alc, b, v, vgen_vint(alc, 0, type_gen(b, alc, "bool"), true), op_eq);
                        }
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
        if (ul)
            ul->read_after_move = true;
        return value_init(alc, v_decl, decl, decl->type);
    }

    if (idf->type == idf_global) {
        Global *g = idf->item;
        Type *rett = g->type;
        rett = type_clone(alc, rett);
        rett->ref = true;
        return value_init(alc, v_global, g, rett);
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
            tok(fc, token, true, false);
            Func *func = map_get(class->funcs, token);
            if (!func) {
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

                value = usage_move_value(alc, fc, scope, value);

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
        int value = (int)(intptr_t)map_get(enu->values, token);

        return vgen_vint(alc, value, type_gen(fc->b, alc, "i32"), false);
    }

    if (idf->type == idf_fc) {
        Fc *rfc = idf->item;

        tok_expect(fc, ".", true, false);
        tok(fc, token, true, false);

        Idf *idf_ = map_get(rfc->scope->identifiers, token);
        if (!idf_) {
            sprintf(fc->sbuf, "Unknown property: '%s'", token);
            fc_error(fc);
        }

        return value_handle_idf(fc, alc, scope, id, idf_);
    }

    sprintf(fc->sbuf, "Cannot convert identifier to a value: '%s'", id->name);
    fc_error(fc);
}

Value *value_op(Fc *fc, Allocator *alc, Scope *scope, Value *left, Value *right, int op) {

    if (left->type == v_vint && right->type == v_vint) {
        VInt *lint = left->item;
        VInt *rint = right->item;
        if (lint->force_type == false && rint->force_type == false) {
            // If both are number literals
            if (op == op_add) {
                lint->value += rint->value;
            } else if (op == op_sub) {
                lint->value -= rint->value;
            }
            return left;
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
                return vgen_fcall(alc, scope, on, values, func->rett, NULL);
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
                long int max = ((long int)1) << bits;
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
            return vgen_cast(alc, val, to_type);
        }
    }

    return val;
}

Value *value_func_call(Allocator *alc, Fc *fc, Scope *scope, Value *on) {
    //
    char *token = fc->token;
    Type *ont = on->rett;
    int ontt = ont->type;

    if (ontt != type_func_ptr) {
        sprintf(fc->sbuf, "Function call on non-function value");
        fc_error(fc);
    }

    Array *args = ont->func_args;
    Type *rett = ont->func_rett;
    Array *errors = ont->func_errors;
    bool can_error = ont->func_can_error;

    if (!args || !rett) {
        sprintf(fc->sbuf, "Function pointer value is missing function type information (compiler bug)");
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

            if (!arg->type->borrow) {
                first_val = usage_move_value(alc, fc, scope, first_val);
            }

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
                if (!arg->type->borrow) {
                    val = usage_move_value(alc, fc, scope, val);
                }

                type_check(fc, arg->type, val->rett);
                array_push(values, val);

                tok(fc, token, false, true);
                if (strcmp(token, ",") == 0) {
                    continue;
                }
                if (strcmp(token, ")") != 0) {
                    sprintf(fc->sbuf, "Expected ',' or ')'");
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

            if (strcmp(token, "!!") == 0) {
                // !!
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

            } else if (strcmp(token, "!?") == 0) {
                // !?
                Value *right = read_value(fc, alc, usage_scope, true, 0, false);
                right = try_convert(fc, alc, right, rett);
                usage_merge_ancestors(alc, scope, ancestors);

                type_check(fc, rett, right->rett);

                or->value = right;

            } else {
                sprintf(fc->sbuf, "The function can return errors, expected '!?', '!!' or '!', but found '%s'", token);
                fc_error(fc);
            }
        }
    }

    return vgen_fcall(alc, scope, on, values, rett, or);
}

bool value_is_assignable(Value *v) {
    //
    return (v->type == v_decl || v->type == v_class_pa || v->type == v_ptrv || v->type == v_global || v->type == v_array_item);
}