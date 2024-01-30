
#include "../all.h"

void stage_4_1_func(Fc *fc, Func *func);
void stage_4_1_gen_main(Fc *fc);
void stage_4_1_gen_test_main(Fc *fc);

void token_declare(Allocator *alc, Fc *fc, Scope *scope, bool replace);
void token_return(Allocator *alc, Fc *fc, Scope *scope);
void token_throw(Allocator *alc, Fc *fc, Scope *scope);
void token_if(Allocator *alc, Fc *fc, Scope *scope);
void token_while(Allocator *alc, Fc *fc, Scope *scope);
void token_each(Allocator *alc, Fc *fc, Scope *scope);

void stage_4_1(Fc *fc) {
    //
    if (fc->is_header)
        return;

    Build *b = fc->b;
    if (b->verbose > 2) {
        printf("# Stage 4.1 : AST : %s\n", fc->path_ki);
    }

    if ((b->main_func && fc == b->main_func->fc) || (!b->main_func && !b->main_fc && fc->nsc == b->nsc_main)) {
        b->main_fc = fc;
        if (b->test) {
            stage_4_1_gen_test_main(fc);
        }
        stage_4_1_gen_main(fc);
    }

    for (int i = 0; i < fc->funcs->length; i++) {
        Func *func = array_get_index(fc->funcs, i);
        if (!func->chunk_body)
            continue;
        if (b->verbose > 2) {
            printf("> Read func AST: %s\n", func->dname);
        }
        stage_4_1_func(fc, func);
    }

    if (b->lsp) {
        alc_wipe(fc->alc_ast);
        return;
    }

    // Write IR
    // stage_4_2(fc);
}

void stage_4_1_func(Fc *fc, Func *func) {
    //
    fc->error_func_info = func;

    if (func->is_generated) {
        return;
    }

    Scope *fscope = func->scope;
    Chunk *chunk = func->chunk_body;
    fc->chunk = chunk;

    if (func->is_test) {
        Allocator *alc = fc->alc_ast;
        Build *b = fc->b;
        Value *right = vgen_vint(alc, 0, type_gen(b, alc, "u32"), false);
        func->test->expects = right->item;
        Arg *arg = array_get_index(func->args, 0);
        Value *argv = vgen_array_item(alc, fscope, value_init(alc, v_decl, arg->decl, arg->type), vgen_vint(alc, 0, type_gen(b, alc, "u32"), false));
        array_push(fscope->ast, tgen_assign(alc, argv, right));
    }

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

            if (decl->is_arg) {
                if (!decl->type->borrow && ul->moves_possible == 0) {
                    sprintf(fc->sbuf, "Argument '%s' passes ownership but it doesnt need it. Add a '*' sign to your argument type to make it a borrow type.", decl->name);
                    fc_error(fc);
                }
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

    bool is_vscope = scope->vscope != NULL;
    const char *end_char = is_vscope ? "}" : "}";

    Chunk *chunk = fc->chunk;
    char *t_ = &chunk->token;

    while (true) {
        //
        tok(fc, token, false, true);

        if (token[0] == 0) {
            sprintf(fc->sbuf, "Unexpected end of file");
            fc_error(fc);
        }


        int t = *t_;
        if (t == tok_cc) {
            rtok(fc);
            skip_whitespace(fc);
            read_macro(fc, alc, scope);
            continue;
        }

        if (scope->did_return) {
            if (single_line) {
                rtok(fc);
                break;
            }
            if (strcmp(token, end_char) != 0) {
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

        if (strcmp(token, end_char) == 0 && !single_line) {
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

        if (strcmp(token, "@expect") == 0) {
            int line = fc->chunk->line;
            int col = fc->chunk->col;

            Func *func = scope->func;
            if (!func->is_test) {
                sprintf(fc->sbuf, "You can only use @expect in a 'test'");
                fc_error(fc);
            }
            Value *val = read_value(fc, alc, scope, false, 0, false);
            if (!type_is_bool(val->rett, fc->b)) {
                sprintf(fc->sbuf, "@expect value must return a bool type");
                fc_error(fc);
            }

            Array *args = array_make(alc, 4);
            array_push(args, val);
            Arg *pass = array_get_index(func->args, 1);
            array_push(args, value_init(alc, v_decl, pass->decl, pass->type));
            Arg *fail = array_get_index(func->args, 2);
            array_push(args, value_init(alc, v_decl, fail->decl, fail->type));

            Func *log = ki_get_func(fc->b, "os", "test_expect");
            Value *fptr = vgen_fptr(alc, log, NULL);
            Value *fcall = vgen_fcall(alc, scope, fptr, args, log->rett, NULL, line, col);
            array_push(scope->ast, token_init(alc, tkn_statement, fcall));

            tok_expect(fc, ";", false, true);
            scope->func->test->expects->value++;
            continue;
        }
        if (strcmp(token, "@move") == 0) {
            Value *on = read_value(fc, alc, scope, true, 0, false);
            on = usage_move_value(alc, fc, scope, on, on->rett);
            array_push(scope->ast, token_init(alc, tkn_statement, on));
            tok_expect(fc, ";", false, true);
            continue;
        }
        if (strcmp(token, "@ref") == 0 || strcmp(token, "@deref") == 0) {
            bool deref = strcmp(token, "@deref") == 0;
            Value *on = read_value(fc, alc, scope, true, 0, false);
            Type *rett = on->rett;
            class_ref_change(alc, scope, on, deref ? -1 : 1, false);
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

                if (left->rett->borrow && left->type != v_ptrv) {
                    if (left->type == v_decl) {
                        Decl *decl = left->item;
                        if (scope != decl->scope) {
                            sprintf(fc->sbuf, "You cannot change the value of a variable that has a borrow type while being inside an if/while/... or any other sub scope.");
                            fc_error(fc);
                        }
                    } else {
                        sprintf(fc->sbuf, "You cannot change the value of a property that has a borrow type");
                        fc_error(fc);
                    }
                }

                if (left->type == v_decl) {
                    Decl *decl = left->item;
                    if (!decl->is_mut) {
                        decl->is_mut = true;
                    }
                }
                if (left->type == v_class_pa) {
                    VClassPA *pa = left->item;
                    if (pa->prop->act != act_public && pa->on->rett->class->fc->nsc != fc->nsc) {
                        if (pa->prop->act == act_readonly) {
                            sprintf(fc->sbuf, "Trying to assign to a readonly property from another namespace");
                        } else {
                            sprintf(fc->sbuf, "Trying to assign to a private property from another namespace");
                        }
                        fc_error(fc);
                    }
                }

                value_disable_upref_deref(left);

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

                if (right->rett->shared_ref && (left->type == v_class_pa || left->type == v_global)) {
                    sprintf(fc->sbuf, "References can only be assigned to local variables");
                    fc_error(fc);
                }
                if (left->type != v_ptrv) {
                    right = usage_move_value(alc, fc, scope, right, left->rett);
                }
                if (left->type == v_class_pa) {
                    // Check if assigning to moved value
                    VClassPA *pa = left->item;
                    Value *on = pa->on;
                    if (on->type == v_decl) {
                        Decl *decl = on->item;
                        UsageLine *ul = usage_line_get(scope, decl);
                        if (ul && ul->moves > 0) {
                            sprintf(fc->sbuf, "You cannot assign a value to a property of a moved value (variable: %s)", decl->name);
                            fc_error(fc);
                        }
                    }
                }

                Value *ir_right = vgen_ir_val(alc, right, right->rett);
                array_push(scope->ast, token_init(alc, tkn_ir_val, ir_right->item));

                if (left->type == v_decl) {
                    Decl *decl = left->item;
                    UsageLine *ul = usage_line_get(scope, decl);
                    if (ul) {
                        end_usage_line(alc, ul, ul->scope->ast);
                    }
                } else if (left->type == v_class_pa || left->type == v_global) {
                    // Deref
                    Value *on = vgen_value_then_ir_value(alc, left);
                    class_ref_change(alc, scope, on, -1, left->rett->weak_ptr);
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

        if (!type_is_void(rett) && class && class->is_rc) {
            sprintf(fc->sbuf, "Statement returns a value, but no variable to store it in");
            fc_error(fc);
        }

        array_push(scope->ast, token_init(alc, tkn_statement, val));
        tok_expect(fc, ";", false, true);
    }

    // Derefs
    if (!scope->did_return) {
        deref_expired_decls(alc, scope, scope->ast);
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
        if (type->weak_ptr) {
            type = type_clone(alc, type);
            type->weak_ptr = false;
        }
    }

    val = usage_move_value(alc, fc, scope, val, type);

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
        if (type_is_void(val->rett)) {
            sprintf(fc->sbuf, "You cannot return a void value inside a value scope '<{'");
            fc_error(fc);
        }
        if (rett) {
            if (rett->nullable || val->rett->nullable) {
                if (!rett->nullable) {
                    rett = type_clone(alc, rett);
                    rett->nullable = true;
                    vscope->vscope->rett = rett;
                }
                if (!val->rett->nullable) {
                    val->rett = type_clone(alc, val->rett);
                    val->rett->nullable = true;
                }
            }
            val = try_convert(fc, alc, val, rett);

            type_check(fc, rett, val->rett);
        } else {
            rett = val->rett;
            vscope->vscope->rett = rett;
        }

        val = usage_move_value(alc, fc, scope, val, rett);

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

        val = usage_move_value(alc, fc, scope, val, frett);

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
    char *err_name = dups(alc, token);

    Throw *throw = al(alc, sizeof(Throw));
    throw->func = func;
    throw->code = index + 1;
    throw->msg = NULL;

    tok(fc, token, true, true);
    if (strcmp(token, ",") == 0) {
        Value *msg = read_value(fc, alc, scope, false, 0, false);
        if (msg->type != v_string) {
            sprintf(fc->sbuf, "Throw message must be a static string value");
            fc_error(fc);
        }
        throw->msg = msg->item;
    } else {
        throw->msg = err_name;
        rtok(fc);
    }

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
    bool single = false;
    if (strcmp(token, "{") == 0) {
    } else if (strcmp(token, ":") == 0) {
        single = true;
    } else {
        sprintf(fc->sbuf, "Expected '{' (scope) or ':' (single line) after the if-condition");
        fc_error(fc);
    }

    Scope *sub = usage_scope_init(alc, scope, sct_default);
    Scope *else_scope = usage_scope_init(alc, scope, sct_default);

    scope_apply_issets(alc, sub, cond->issets);

    read_ast(fc, sub, single);

    tok(fc, token, false, true);
    if (strcmp(token, "else") == 0 && fc->chunk->token != tok_cc) {
        tok(fc, token, true, true);
        bool has_if = strcmp(token, "if") == 0;
        if (has_if) {
            token_if(alc, fc, else_scope);
        } else {
            rtok(fc);
            tok(fc, token, false, true);
            bool single = false;
            if (strcmp(token, "{") == 0) {
            } else if (strcmp(token, ":") == 0) {
                single = true;
            } else {
                sprintf(fc->sbuf, "Expected '{' (scope) or ':' (single line) after 'else' token");
                fc_error(fc);
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
    char *token = fc->token;
    Scope *sub = usage_scope_init(alc, scope, sct_loop);
    Value *cond = read_value(fc, alc, sub, true, 0, false);

    if (!type_is_bool(cond->rett, fc->b)) {
        sprintf(fc->sbuf, "Value must return a bool type");
        fc_error(fc);
    }

    tok(fc, token, false, true);
    bool single = false;
    if (strcmp(token, "{") == 0) {
    } else if (strcmp(token, ":") == 0) {
        single = true;
    } else {
        sprintf(fc->sbuf, "Expected '{' (scope) or ':' (single line) after the while-condition");
        fc_error(fc);
    }

    scope_apply_issets(alc, sub, cond->issets);

    read_ast(fc, sub, single);

    // usage_clear_ancestors(scope);
    Array *ancestors = array_make(alc, 2);
    array_push(ancestors, sub);
    usage_merge_ancestors(alc, scope, ancestors);

    array_push(scope->ast, tgen_while(alc, cond, sub));
}

void token_each(Allocator *alc, Fc *fc, Scope *scope) {
    //
    int col = fc->chunk->col;
    int line = fc->chunk->line;

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
    bool single = false;
    if (strcmp(token, "{") == 0) {
    } else if (strcmp(token, ":") == 0) {
        single = true;
    } else {
        sprintf(fc->sbuf, "Expected '{' (scope) or ':' (single line) after the each-condition");
        fc_error(fc);
    }

    read_ast(fc, sub, single);

    Array *ancestors = array_make(alc, 2);
    array_push(ancestors, sub);
    usage_merge_ancestors(alc, scope, ancestors);

    array_push(scope->ast, tgen_each(alc, on, sub, decl_k, decl_v, line, col));
}

void stage_4_1_gen_main(Fc *fc) {
    //
    Build *b = fc->b;
    Allocator *alc = fc->alc;

    Chunk *chunk = chunk_init(alc, fc);
    Func *mfunc = b->main_func;
    if (mfunc) {
        // Validate main
        fc->chunk = mfunc->chunk_args;
        if (mfunc->args->length > 1) {
            sprintf(fc->sbuf, "Function 'main' has too many arguments");
            fc_error(fc);
        }

        if (mfunc->args->length > 0) {
            Arg *arg = array_get_index(mfunc->args, 0);
            chunk->content = "*Array[String]";
            chunk->length = 100;
            fc->chunk = chunk;

            Type *type = read_type(fc, alc, fc->scope, true, true, rtc_func_arg);
            TypeCheck *tc = type_gen_type_check(alc, type);

            fc->chunk = arg->type_chunk;
            type_validate(fc, tc, arg->type, "First argument of 'main' should be different");
        }
    }

    Func *func = func_init(fc->alc, fc->b);
    func->fc = fc;
    func->name = "main";
    func->gname = "main";
    func->dname = "main";
    func->scope = scope_init(fc->alc, sct_func, fc->scope, true);
    func->scope->func = func;
    func->will_exit = false;
    func->rett = type_gen(b, alc, "i32");

    array_push(fc->funcs, func);

    // Args
    Arg *arg = arg_init(alc, "argc", type_gen(b, alc, "i32"));
    arg->value_chunk = NULL;
    arg->type_chunk = NULL;
    array_push(func->args, arg);
    map_set(func->args_by_name, "argc", arg);

    arg = arg_init(alc, "argv", type_gen(b, alc, "ptr"));
    arg->value_chunk = NULL;
    arg->type_chunk = NULL;
    array_push(func->args, arg);
    map_set(func->args_by_name, "argv", arg);

    func_make_arg_decls(func);

    // Ast
    Scope *scope = func->scope;

    bool main_has_return = mfunc && !type_is_void(mfunc->rett);
    bool main_has_arg = mfunc && mfunc->args->length > 0;

    Str *code = str_make(alc, 1000);
    str_append_chars(code, "{\n");
    str_append_chars(code, "let arr = Array[String].new();\n");
    str_append_chars(code, "let i = 0;\n");
    str_append_chars(code, "while i < argc {\n");
    str_append_chars(code, "let cstr = @ptrv(argv, c_string, i);\n");
    str_append_chars(code, "arr.push(cstr.to_str());\n");
    str_append_chars(code, "i++;\n");
    str_append_chars(code, "}\n");

    if (b->test) {
        str_append_chars(code, "ki__test__main();\n");
        str_append_chars(code, "return 0;\n");
    } else {
        if (b->main_func) {
            if (main_has_return)
                str_append_chars(code, "return ");
            str_append_chars(code, "main(");
            if (main_has_arg) {
                str_append_chars(code, "arr");
            }
            str_append_chars(code, ");\n");
        }
        if (!main_has_return)
            str_append_chars(code, "return 0;\n");
    }

    str_append_chars(code, "}\n");

    chunk->content = str_to_chars(alc, code);
    chunk->length = code->length;
    chunk->i = 0;
    fc->chunk = chunk;
    chunk_lex_start(chunk);
    // Skip first character
    tok(fc, NULL, false, true);

    read_ast(fc, scope, false);
}

void stage_4_1_gen_test_main(Fc *fc) {
    //
    Allocator *alc = fc->alc_ast;
    Func *func = func_init(alc, fc->b);
    Build *b = fc->b;

    char *name = "ki__test__main";
    char *gname = name;
    char *dname = "ki:test:main";

    func->fc = fc;
    func->name = name;
    func->gname = gname;
    func->dname = dname;
    func->scope = scope_init(alc, sct_func, fc->scope, true);
    func->scope->func = func;
    func->rett = b->type_void;

    Idf *idf = idf_init(fc->alc, idf_func);
    idf->item = func;
    map_set(fc->nsc->scope->identifiers, func->name, idf);

    array_push(fc->funcs, func);

    Scope *scope = func->scope;
    Array *tests = fc->b->tests;

    char line[1024];
    map_set(scope->identifiers, "os_test_print_name", ki_lib_get(b, "os", "test_print_name"));
    map_set(scope->identifiers, "os_test_report", ki_lib_get(b, "os", "test_report"));

    Str *code = str_make(alc, 5000);
    str_append_chars(code, "let test_success : u32 = 0;\n");
    str_append_chars(code, "let test_fail : u32 = 0;\n");
    str_append_chars(code, "let expect_total : u32 = 0;\n");
    str_append_chars(code, "let expect_success : u32 = 0;\n");
    str_append_chars(code, "let expect_fail : u32 = 0;\n");

    str_append_chars(code, "print(\"\\n\");\n");

    for (int i = 0; i < tests->length; i++) {
        Test *test = array_get_index(tests, i);
        map_set(scope->identifiers, test->func->gname, idf_init_item(fc->alc_ast, idf_func, test->func));

        sprintf(line, "let expect_count_%d : u32 = 0;\n", i);
        str_append_chars(code, line);
        sprintf(line, "let expect_success_%d : u32 = 0;\n", i);
        str_append_chars(code, line);
        sprintf(line, "let expect_fail_%d : u32 = 0;\n", i);
        str_append_chars(code, line);
        // Call test
        sprintf(line, "%s(@array_of(expect_count_%d), @array_of(expect_success_%d), @array_of(expect_fail_%d));\n", test->func->gname, i, i, i);
        str_append_chars(code, line);
        // Check result
        sprintf(line, "os_test_print_name(\"%s\", expect_fail_%d == 0);\n", test->name, i);
        str_append_chars(code, line);
        sprintf(line, "if expect_fail_%d == 0 : test_success++; else: test_fail++;\n", i);
        str_append_chars(code, line);
        sprintf(line, "expect_total += expect_count_%d;\n", i);
        str_append_chars(code, line);
        sprintf(line, "expect_success += expect_success_%d;\n", i);
        str_append_chars(code, line);
        sprintf(line, "expect_fail += expect_fail_%d;\n", i);
        str_append_chars(code, line);
    }

    char nr[10];
    sprintf(nr, "%d", tests->length);

    str_append_chars(code, "os_test_report(");
    str_append_chars(code, nr);
    str_append_chars(code, ", test_success, test_fail, expect_total, expect_success, expect_fail);\n");

    str_append_chars(code, "}\n");

    Chunk *chunk = chunk_init(alc, fc);
    chunk->content = str_to_chars(alc, code);
    chunk->length = code->length;
    chunk_lex_start(chunk);

    func->chunk_body = chunk;
}
