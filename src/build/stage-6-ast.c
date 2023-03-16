
#include "../all.h"

void stage_6_func(Fc *fc, Func *func);

void token_declare(Allocator *alc, Fc *fc, Scope *scope, bool replace);
void token_return(Allocator *alc, Fc *fc, Scope *scope);
TIf *token_if(Allocator *alc, Fc *fc, Scope *scope, bool has_cond, Array **used_decls);
void token_while(Allocator *alc, Fc *fc, Scope *scope);

void stage_6(Fc *fc) {
    //
    if (fc->is_header)
        return;

    Build *b = fc->b;
    if (b->verbose > 1) {
        printf("# Stage 6 : AST : %s\n", fc->path_ki);
    }

    for (int i = 0; i < fc->funcs->length; i++) {
        Func *func = array_get_index(fc->funcs, i);
        if (!func->chunk_body)
            continue;
        if (b->verbose > 1) {
            printf("> Read func AST: %s\n", func->dname);
        }
        stage_6_func(fc, func);
    }

    // Write IR
    stage_7(fc);
}

void stage_6_func(Fc *fc, Func *func) {
    //

    if (func->is_generated) {
        return;
    }

    Chunk *chunk = func->chunk_body;
    fc->chunk = chunk;

    read_ast(fc, func->scope, false);

    if (!type_is_void(func->rett) && !func->scope->did_return) {
        sprintf(fc->sbuf, "Function did not return a value");
        fc_error(fc);
    }
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
            Array *used_decls = NULL;
            TIf *tif = token_if(alc, fc, scope, true, &used_decls);
            array_push(scope->ast, token_init(alc, tkn_if, tif));

            if (scope->usage_keys) {
                TIf *tif_ = tif;
                while (tif_) {
                    Scope *sub = tif_->scope;
                    usage_merge_scopes(alc, scope, sub, used_decls);
                    tif_ = tif_->else_if;
                }
            }
            continue;
        }

        if (strcmp(token, "while") == 0) {
            token_while(alc, fc, scope);
            continue;
        }

        rtok(fc);

        Chunk *before = chunk_clone(alc, fc->chunk);

        // Check if var assign
        Value *left = NULL;

        Id *id = read_id(fc, false, true, true);
        Idf *idf = idf_by_id(fc, scope, id, false);
        if (idf && idf->type == idf_var) {
            tok(fc, token, false, true);
            if (strcmp(token, "=") == 0) {
                Var *var = idf->item;
                left = value_init(alc, v_var, var, var->type);
            }
            rtok(fc);
        }

        //
        if (!left) {
            fc->chunk = before;
            left = read_value(fc, alc, scope, false, 0);
        }

        // Assign
        if (left->type == v_var || left->type == v_class_pa || left->type == v_ptrv) {
            tok(fc, token, false, true);
            sprintf(fc->sbuf, ".%s.", token);
            if (strstr(".=.+=.-=.*=./=.", fc->sbuf)) {
                char *sign = dups(alc, token);

                if (left->type == v_var) {
                    Var *var = left->item;
                    Decl *decl = var->decl;
                    if (!decl->is_mut) {
                        sprintf(fc->sbuf, "Cannot assign value to an immutable variable");
                        fc_error(fc);
                    }
                }
                if (left->type == v_class_pa) {
                    VClassPA *pa = left->item;
                    ClassProp *prop = pa->prop;
                    // if prop.act != AccessType.public {
                    // 	if !scope.is_sub_scope_of(prop.class.scope) {
                    // 		fc.error("Trying to assign to a non-public property outside the class");
                    // 	}
                    // }
                }

                Value *right = read_value(fc, alc, scope, false, 0);
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

                if (left->type == v_var) {
                    Var *var = idf->item;
                    Decl *decl = var->decl;
                    Scope *sub = scope_init(alc, sct_default, scope, true);
                    Value *val = value_init(alc, v_decl, decl, decl->type);
                    UsageLine *ul = usage_line_get(scope, decl);
                    class_ref_change(alc, sub, val, -1);
                    array_push(scope->ast, tgen_exec_unless_moved_once(alc, sub, ul));
                }

                array_push(scope->ast, tgen_assign(alc, left, right));
                tok_expect(fc, ";", false, true);

                if (left->type == v_var) {
                    Var *var = idf->item;
                    Decl *decl = var->decl;
                    usage_line_init(alc, scope, decl);
                }

                continue;
            }
            rtok(fc);
        }

        // Statement
        Value *val = left;

        if (!type_is_void(val->rett)) {
            sprintf(fc->sbuf, "Statement returns a value, but no variable to store it in");
            fc_error(fc);
        }

        array_push(scope->ast, token_init(alc, tkn_statement, val));
        tok_expect(fc, ";", false, true);
    }

    // Derefs
    if (!scope->did_return) {
        deref_scope(alc, scope, scope);
    }
}

void token_declare(Allocator *alc, Fc *fc, Scope *scope, bool replace) {
    //
    char *token = fc->token;

    tok(fc, token, true, true);

    bool mutable = false;

    if (strcmp(token, "mut") == 0) {
        mutable = true;
        tok(fc, token, true, true);
    }
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
        type = read_type(fc, alc, scope, false, true);
        tok(fc, token, false, true);
    }

    if (strcmp(token, "=") != 0) {
        sprintf(fc->sbuf, "Expected '='");
        fc_error(fc);
    }

    Value *val = read_value(fc, alc, scope, false, 0);

    if (type) {
        val = try_convert(fc, alc, val, type);
        type_check(fc, type, val->rett);
    } else {
        type = val->rett;
    }

    if (type_is_void(type)) {
        sprintf(fc->sbuf, "Variable declaration: Right side does not return a value");
        fc_error(fc);
    }

    tok_expect(fc, ";", false, true);

    Decl *decl = decl_init(alc, scope, name, type, val, mutable, false, false);
    array_push(scope->ast, token_init(alc, tkn_declare, decl));

    Var *var = var_init(alc, decl, type);

    Idf *idf = idf_init(alc, idf_var);
    idf->item = var;

    map_set(scope->identifiers, name, idf);

    usage_line_init(alc, scope, decl);
}

void token_return(Allocator *alc, Fc *fc, Scope *scope) {
    //
    Scope *fscope = scope_find(scope, sct_func);
    if (!fscope) {
        sprintf(fc->sbuf, "Using 'return' outside function scope");
        fc_error(fc);
    }

    Func *func = fscope->func;
    Type *frett = func->rett;
    Value *retval = NULL;

    if (!type_is_void(frett)) {
        Value *val = read_value(fc, alc, scope, true, 0);
        val = try_convert(fc, alc, val, frett);
        type_check(fc, frett, val->rett);

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

TIf *token_if(Allocator *alc, Fc *fc, Scope *scope, bool has_cond, Array **used_decls) {
    //
    char *token = fc->token;
    Value *cond = NULL;

    if (has_cond) {
        Value *val = read_value(fc, alc, scope, true, 0);
        if (!type_is_bool(val->rett, fc->b)) {
            sprintf(fc->sbuf, "Condition value must return a bool");
            fc_error(fc);
        }
        cond = val;
    }

    tok(fc, token, false, true);
    bool single = strcmp(token, "{") != 0;
    if (single) {
        rtok(fc);
    }

    Scope *sub = usage_scope_init(alc, scope, sct_default);

    read_ast(fc, sub, single);

    usage_collect_used_decls(alc, scope, sub, used_decls);

    tok(fc, token, false, true);
    TIf *else_if = NULL;
    if (strcmp(token, "else") == 0) {

        tok(fc, token, true, true);
        bool has_if = strcmp(token, "if") == 0;
        if (!has_if) {
            rtok(fc);
        }
        else_if = token_if(alc, fc, scope, has_if, used_decls);

    } else {
        // Generate else for usage algorithm
        if (has_cond) {
            Scope *sub = usage_scope_init(alc, scope, sct_default);
            else_if = tgen_tif(alc, NULL, sub, NULL);
        }
        rtok(fc);
    }

    return tgen_tif(alc, cond, sub, else_if);
}

void token_while(Allocator *alc, Fc *fc, Scope *scope) {
    //
    Scope *sub = usage_scope_init(alc, scope, sct_loop);
    // Scope *sub = scope_init(alc, sct_loop, scope, true);

    Value *cond = read_value(fc, alc, sub, true, 0);

    if (!type_is_bool(cond->rett, fc->b)) {
        sprintf(fc->sbuf, "Value must return a bool type");
        fc_error(fc);
    }

    tok_expect(fc, "{", false, true);

    read_ast(fc, sub, false);

    array_push(scope->ast, tgen_while(alc, cond, sub));
}
