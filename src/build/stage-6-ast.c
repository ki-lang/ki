
#include "../all.h"

void stage_6_func(Fc *fc, Func *func);

void token_declare(Allocator *alc, Fc *fc, Scope *scope, bool replace);
void token_return(Allocator *alc, Fc *fc, Scope *scope);
void token_throw(Allocator *alc, Fc *fc, Scope *scope);
void token_if(Allocator *alc, Fc *fc, Scope *scope);
void token_while(Allocator *alc, Fc *fc, Scope *scope);
void token_each(Allocator *alc, Fc *fc, Scope *scope);

void stage_6(Fc *fc) {
    //
    if (fc->is_header)
        return;

    Build *b = fc->b;
    if (b->verbose > 2) {
        printf("# Stage 6 : AST : %s\n", fc->path_ki);
    }

    for (int i = 0; i < fc->funcs->length; i++) {
        Func *func = array_get_index(fc->funcs, i);
        if (!func->chunk_body)
            continue;
        if (b->verbose > 2) {
            printf("> Read func AST: %s\n", func->dname);
        }
        stage_6_func(fc, func);
    }

    // Write IR
    stage_7(fc);
}

void stage_6_func(Fc *fc, Func *func) {
    //
    fc->error_func_info = func;

    if (func->is_generated) {
        return;
    }

    Scope *fscope = func->scope;
    Chunk *chunk = func->chunk_body;
    fc->chunk = chunk;

    read_ast(fc, func->scope, false);

    if (!type_is_void(func->rett) && !fscope->did_return) {
        sprintf(fc->sbuf, "Function did not return a value");
        fc_error(fc);
    }

    // Check unused ownership args
    Array *decls = fscope->usage_keys;
    if (decls) {
        for (int i = 0; i < decls->length; i++) {
            Decl *decl = array_get_index(decls, i);
            UsageLine *ul = array_get_index(fscope->usage_values, i);

            if (decl->is_arg && !decl->type->borrow && ul->moves == 0) {
                sprintf(fc->sbuf, "Argument '%s' passes ownership but it doesnt need it. Remove the '>' sign from your argument type.", decl->name);
                fc_error(fc);
            }
        }
    }

    fc->error_func_info = NULL;
}

void read_ast(Fc *fc, Scope *scope, bool single_line) {
    //
    char *token = fc->token;
    bool first_line = true;
    Allocator *alc = fc->alc_ast;

    while (true) {
        //
        tok(fc, token, false, true);

        if (token[0] == 0) {
            sprintf(fc->sbuf, "Unexpected end of file");
            fc_error(fc);
        }

        if (strcmp(token, "#") == 0) {
            read_macro(fc, alc, scope);
            continue;
        }

        if (scope->did_return) {
            if (single_line) {
                rtok(fc);
                break;
            }
            if (strcmp(token, "}") != 0) {
                sprintf(fc->sbuf, "Expected '}'");
                fc_error(fc);
            }
            break;
        }

        if (single_line && !first_line) {
            rtok(fc);
            break;
        }
        first_line = false;

        if (strcmp(token, "}") == 0 && !single_line) {
            break;
        }

        if (strcmp(token, "let") == 0) {
            token_declare(alc, fc, scope, false);
            continue;
        }
        if (strcmp(token, "rep") == 0) {
            token_declare(alc, fc, scope, true);
            continue;
        }

        if (strcmp(token, "return") == 0) {
            token_return(alc, fc, scope);
            continue;
        }
        if (strcmp(token, "throw") == 0) {
            token_throw(alc, fc, scope);
            continue;
        }
        if (strcmp(token, "each") == 0) {
            token_each(alc, fc, scope);
            continue;
        }
        if (strcmp(token, "break") == 0) {
            Scope *loop = scope_find(scope, sct_loop);
            if (!loop) {
                sprintf(fc->sbuf, "You can only use 'break' inside a loop");
                fc_error(fc);
            }
            deref_scope(alc, scope, loop);
            array_push(scope->ast, token_init(alc, tkn_break, loop));
            tok_expect(fc, ";", false, true);
            scope->did_return = true;
            continue;
        }
        if (strcmp(token, "continue") == 0) {
            Scope *loop = scope_find(scope, sct_loop);
            if (!loop) {
                sprintf(fc->sbuf, "You can only use 'break' inside a loop");
                fc_error(fc);
            }
            deref_scope(alc, scope, loop);
            array_push(scope->ast, token_init(alc, tkn_continue, loop));
            tok_expect(fc, ";", false, true);
            scope->did_return = true;
            continue;
        }

        if (strcmp(token, "if") == 0) {
            token_if(alc, fc, scope);
            continue;
        }

        if (strcmp(token, "while") == 0) {
            token_while(alc, fc, scope);
            continue;
        }
        if (strcmp(token, "@move") == 0) {
            Value *on = read_value(fc, alc, scope, true, 0, false);
            on = usage_move_value(alc, fc, scope, on);
            array_push(scope->ast, token_init(alc, tkn_statement, on));
            tok_expect(fc, ";", false, true);
            continue;
        }
        if (strcmp(token, "@ref") == 0 || strcmp(token, "@deref") == 0) {
            bool deref = strcmp(token, "@deref") == 0;
            Value *on = read_value(fc, alc, scope, true, 0, false);
            Type *rett = on->rett;
            class_ref_change(alc, scope, on, deref ? -1 : 1);
            tok_expect(fc, ";", false, true);
            continue;
        }

        rtok(fc);

        // printf("%s | %d\n", fc->path_ki, fc->chunk->line);
        Value *left = read_value(fc, alc, scope, false, 0, true);

        // Assign
        if (value_is_assignable(left)) {
            tok(fc, token, false, true);
            sprintf(fc->sbuf, ".%s.", token);
            if (strstr(".=.+=.-=.*=./=.", fc->sbuf)) {
                char *sign = dups(alc, token);

                if (left->rett->imut) {
                    if (left->type == v_class_pa || left->type == v_array_item) {
                        sprintf(fc->sbuf, "You cannot modify an immutable object or static array");
                        fc_error(fc);
                    }
                }

                if (left->type == v_decl) {
                    Decl *decl = left->item;
                    if (!decl->is_mut) {
                        if (type_tracks_ownership(decl->type) && decl->type->borrow) {
                            sprintf(fc->sbuf, "You cannot mutate variable that contain borrowed values");
                            fc_error(fc);
                        }
                        decl->is_mut = true;
                    }
                }

                Value *right = read_value(fc, alc, scope, false, 0, false);
                if (type_is_void(right->rett)) {
                    sprintf(fc->sbuf, "Assignment invalid, right side does not return a value");
                    fc_error(fc);
                }

                if (strcmp(sign, "=") == 0) {
                } else if (strcmp(sign, "+=") == 0) {
                    right = value_op(fc, alc, scope, left, right, op_add);
                } else if (strcmp(sign, "-=") == 0) {
                    right = value_op(fc, alc, scope, left, right, op_sub);
                } else if (strcmp(sign, "*=") == 0) {
                    right = value_op(fc, alc, scope, left, right, op_mul);
                } else if (strcmp(sign, "/=") == 0) {
                    right = value_op(fc, alc, scope, left, right, op_div);
                }

                right = try_convert(fc, alc, right, left->rett);
                type_check(fc, left->rett, right->rett);

                if (left->type != v_ptrv) {
                    right = usage_move_value(alc, fc, scope, right);
                }

                Value *ir_right = vgen_ir_val(alc, right, right->rett);
                array_push(scope->ast, token_init(alc, tkn_ir_val, ir_right->item));

                if (left->type == v_decl) {
                    Decl *decl = left->item;
                    UsageLine *ul = usage_line_get(scope, decl);
                    if (ul)
                        end_usage_line(alc, ul);
                } else if (left->type == v_class_pa || left->type == v_global) {
                    // Deref
                    class_ref_change(alc, scope, left, -1);
                }

                array_push(scope->ast, tgen_assign(alc, left, ir_right));
                tok_expect(fc, ";", false, true);

                if (left->type == v_decl) {
                    Decl *decl = left->item;
                    usage_line_init(alc, scope, decl);
                }

                continue;
            }
            rtok(fc);
        }

        // Statement
        Value *val = left;
        Type *rett = val->rett;
        Class *class = rett->class;

        if (!type_is_void(rett) && class && (class->must_deref || class->must_ref)) {
            sprintf(fc->sbuf, "Statement returns a value, but no variable to store it in");
            fc_error(fc);
        }

        array_push(scope->ast, token_init(alc, tkn_statement, val));
        tok_expect(fc, ";", false, true);
    }

    // Derefs
    if (!scope->did_return) {
        deref_expired_decls(alc, scope);
    }
}

void token_declare(Allocator *alc, Fc *fc, Scope *scope, bool replace) {
    //
    char *token = fc->token;

    tok(fc, token, true, true);

    if (is_valid_varname_char(token[0])) {
        sprintf(fc->sbuf, "Invalid variable name syntax '%s'", token);
    }
    if (replace) {
        Idf *idf = map_get(scope->identifiers, token);
        if (!idf) {
            sprintf(fc->sbuf, "Variable not found, nothing to replace '%s'", token);
            fc_error(fc);
        }
        map_unset(scope->upref_slots, token);
    } else {
        name_taken_check(fc, scope, token);
    }

    char *name = dups(alc, token);

    if (map_get(scope->identifiers, name)) {
        sprintf(fc->sbuf, "Variable name already used: '%s'", name);
    }

    Type *type = NULL;

    tok(fc, token, false, true);
    if (strcmp(token, ":") == 0) {
        type = read_type(fc, alc, scope, false, true, rtc_decl);
        tok(fc, token, false, true);
    }

    if (strcmp(token, "=") != 0) {
        sprintf(fc->sbuf, "Expected '='");
        fc_error(fc);
    }

    Value *val = read_value(fc, alc, scope, false, 0, false);

    if (type) {
        val = try_convert(fc, alc, val, type);
        type_check(fc, type, val->rett);
    } else {
        type = val->rett;
    }

    val = usage_move_value(alc, fc, scope, val);

    if (type_is_void(type)) {
        sprintf(fc->sbuf, "Variable declaration: Right side does not return a value");
        fc_error(fc);
    }

    tok_expect(fc, ";", false, true);

    Decl *decl = decl_init(alc, scope, name, type, val, false);
    array_push(scope->ast, token_init(alc, tkn_declare, decl));

    Idf *idf = idf_init(alc, idf_decl);
    idf->item = decl;

    map_set(scope->identifiers, name, idf);

    usage_line_init(alc, scope, decl);
}

void token_return(Allocator *alc, Fc *fc, Scope *scope) {
    //
    Scope *vscope = scope_find_return_scope(scope);

    if (vscope->type == sct_vscope) {
        // Value scope
        Type *rett = vscope->vscope->rett;
        Value *val = read_value(fc, alc, scope, true, 0, false);
        val = try_convert(fc, alc, val, rett);

        type_check(fc, rett, val->rett);

        val = usage_move_value(alc, fc, scope, val);

        IRVal *tvar = al(alc, sizeof(IRVal));
        tvar->value = val;
        tvar->ir_value = NULL;
        Token *t = token_init(alc, tkn_ir_val, tvar);
        array_push(scope->ast, t);

        Value *ret_val = value_init(alc, v_ir_val, tvar, val->rett);

        deref_scope(alc, scope, vscope);

        array_push(scope->ast, tgen_vscope_return(alc, vscope, ret_val));
        tok_expect(fc, ";", false, true);

        scope->did_return = true;
        return;
    }

    Scope *fscope = vscope;
    if (!fscope) {
        sprintf(fc->sbuf, "Using 'return' outside function scope");
        fc_error(fc);
    }

    Func *func = fscope->func;
    Type *frett = func->rett;
    Value *retval = NULL;

    if (!type_is_void(frett)) {
        Value *val = read_value(fc, alc, scope, true, 0, false);
        val = try_convert(fc, alc, val, frett);

        if (scope == fscope) {
            if (val->rett->strict_ownership && !frett->strict_ownership && func->only_returns_strict) {
                sprintf(fc->sbuf, "💡 If you only return strict ownership values then the function return type should be a strict ownership type. You can make a type have strict ownership by placing a '.' in front of it. This is just a forced rule by the language to prevent the problem of someone needing strict ownership at some point and the function not giving it, even though it has strict ownership.");
                fc_error(fc);
            }
        } else {
            if (!val->rett->strict_ownership) {
                func->only_returns_strict = false;
            }
        }

        type_check(fc, frett, val->rett);

        val = usage_move_value(alc, fc, scope, val);

        IRVal *tvar = al(alc, sizeof(IRVal));
        tvar->value = val;
        tvar->ir_value = NULL;
        Token *t = token_init(alc, tkn_ir_val, tvar);
        array_push(scope->ast, t);

        Value *tmp_var = value_init(alc, v_ir_val, tvar, val->rett);
        retval = tmp_var;
    }

    deref_scope(alc, scope, fscope);

    array_push(scope->ast, tgen_return(alc, fscope, retval));
    tok_expect(fc, ";", false, true);

    scope->did_return = true;
}

void token_throw(Allocator *alc, Fc *fc, Scope *scope) {
    //
    Scope *fscope = scope_find(scope, sct_func);
    if (!fscope) {
        sprintf(fc->sbuf, "Using 'throw' outside function scope");
        fc_error(fc);
    }
    Func *func = fscope->func;
    deref_scope(alc, scope, fscope);
    if (!func->can_error) {
        sprintf(fc->sbuf, "You cannot use 'throw' in a function that has no errors defined");
        fc_error(fc);
    }

    char *token = fc->token;
    Array *errors = func->errors;

    if (!func->errors) {
        sprintf(fc->sbuf, "Missing list of errors (compiler bug)");
        fc_error(fc);
    }

    tok(fc, token, true, true);
    int index = array_find(errors, token, arr_find_str);
    if (index < 0) {
        sprintf(fc->sbuf, "The function has no error named '%s'", token);
        fc_error(fc);
    }

    Throw *throw = al(fc->alc, sizeof(Throw));
    throw->func = func;
    throw->code = index + 1;

    array_push(scope->ast, token_init(alc, tkn_throw, throw));
    tok_expect(fc, ";", false, true);

    scope->did_return = true;
}

void token_if(Allocator *alc, Fc *fc, Scope *scope) {
    //
    char *token = fc->token;

    Value *cond = read_value(fc, alc, scope, true, 0, false);
    if (!type_is_bool(cond->rett, fc->b)) {
        sprintf(fc->sbuf, "Condition value must return a bool");
        fc_error(fc);
    }

    tok(fc, token, false, true);
    bool single = strcmp(token, "{") != 0;
    if (single) {
        rtok(fc);
    }

    Scope *sub = usage_scope_init(alc, scope, sct_default);
    Scope *else_scope = usage_scope_init(alc, scope, sct_default);

    scope_apply_issets(alc, sub, cond->issets);

    read_ast(fc, sub, single);

    tok(fc, token, false, true);
    if (strcmp(token, "else") == 0) {
        tok(fc, token, true, true);
        bool has_if = strcmp(token, "if") == 0;
        if (has_if) {
            token_if(alc, fc, else_scope);
        } else {
            rtok(fc);
            tok(fc, token, false, true);
            bool single = strcmp(token, "{") != 0;
            if (single) {
                rtok(fc);
            }
            read_ast(fc, else_scope, single);
        }
    } else {
        rtok(fc);
    }

    Scope *deref_scope = usage_create_deref_scope(alc, scope);

    Array *ancestors = array_make(alc, 2);
    array_push(ancestors, sub);
    array_push(ancestors, else_scope);
    usage_merge_ancestors(alc, scope, ancestors);

    TIf *tif = tgen_tif(alc, cond, sub, else_scope, deref_scope);
    array_push(scope->ast, token_init(alc, tkn_if, tif));
}

void token_while(Allocator *alc, Fc *fc, Scope *scope) {
    //
    Scope *sub = usage_scope_init(alc, scope, sct_loop);
    // Scope *sub = scope_init(alc, sct_loop, scope, true);

    Value *cond = read_value(fc, alc, sub, true, 0, false);

    if (!type_is_bool(cond->rett, fc->b)) {
        sprintf(fc->sbuf, "Value must return a bool type");
        fc_error(fc);
    }

    tok_expect(fc, "{", false, true);

    scope_apply_issets(alc, sub, cond->issets);

    read_ast(fc, sub, false);

    // usage_clear_ancestors(scope);
    Array *ancestors = array_make(alc, 2);
    array_push(ancestors, sub);
    usage_merge_ancestors(alc, scope, ancestors);

    array_push(scope->ast, tgen_while(alc, cond, sub));
}

void token_each(Allocator *alc, Fc *fc, Scope *scope) {
    //
    char *token = fc->token;
    Value *value = read_value(fc, alc, scope, true, 0, false);
    Value *on = vgen_ir_val(alc, value, value->rett);
    array_push(scope->ast, token_init(alc, tkn_ir_val, on->item));

    //
    Type *type = value->rett;
    Class *class = type->class;
    bool valid = class ? true : false;
    Func *f_init = NULL;
    Func *f_get = NULL;
    if (valid) {
        f_init = map_get(class->funcs, "__each_init");
        f_get = map_get(class->funcs, "__each");
        valid = f_init && f_get;
    }
    if (!valid) {
        sprintf(fc->sbuf, "Cannot use 'each' on this value. The type has no iteration functions.");
        fc_error(fc);
    }

    tok_expect(fc, "as", false, true);
    tok(fc, token, false, true);
    if (!is_valid_varname(token)) {
        sprintf(fc->sbuf, "Invalid variable name '%s'", token);
        fc_error(fc);
    }
    name_taken_check(fc, scope, token);

    char *key_name = NULL;
    char *value_name = dups(alc, token);

    tok(fc, token, false, true);
    if (strcmp(token, ",") == 0) {
        key_name = value_name;

        tok(fc, token, false, true);
        if (!is_valid_varname(token)) {
            sprintf(fc->sbuf, "Invalid variable name '%s'", token);
            fc_error(fc);
        }
        if (strcmp(token, value_name) == 0) {
            sprintf(fc->sbuf, "Variable name already used '%s'", token);
            fc_error(fc);
        }
        name_taken_check(fc, scope, token);
        value_name = dups(alc, token);

        tok(fc, token, false, true);
    }

    // Declare next_key
    Decl *decl_nk = decl_init(alc, scope, "each_next_key", f_init->rett, NULL, false);
    UsageLine *ul_nk = usage_line_init(alc, scope, decl_nk);

    // New sub scope
    Scope *sub = usage_scope_init(alc, scope, sct_loop);

    // Declare scope variables
    Decl *decl_k = decl_init(alc, sub, key_name, f_init->rett, NULL, false);
    UsageLine *ul_k = usage_line_init(alc, sub, decl_k);
    Decl *decl_v = decl_init(alc, sub, value_name, f_get->rett, NULL, false);
    UsageLine *ul_v = usage_line_init(alc, sub, decl_v);

    //
    // usage_move_value(alc, fc, sub, on);
    // usage_move_value(alc, fc, sub, );

    //
    if (key_name) {
        Idf *idf = idf_init(alc, idf_decl);
        idf->item = decl_k;
        map_set(sub->identifiers, key_name, idf);
    }

    Idf *idf = idf_init(alc, idf_decl);
    idf->item = decl_v;
    map_set(sub->identifiers, value_name, idf);

    //
    bool single_line = strcmp(token, ":") == 0;
    if (!single_line && strcmp(token, "{") != 0) {
        sprintf(fc->sbuf, "Expected '{' or ':' here, found '%s'", token);
        fc_error(fc);
    }

    read_ast(fc, sub, single_line);

    Array *ancestors = array_make(alc, 2);
    array_push(ancestors, sub);
    usage_merge_ancestors(alc, scope, ancestors);

    array_push(scope->ast, tgen_each(alc, on, sub, decl_k, decl_v));
}
