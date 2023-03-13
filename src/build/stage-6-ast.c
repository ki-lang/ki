
#include "../all.h"

void stage_6_func(Fc *fc, Func *func);

void token_declare(Allocator *alc, Fc *fc, Scope *scope, bool replace);
void token_return(Allocator *alc, Fc *fc, Scope *scope);
TIf *token_if(Allocator *alc, Fc *fc, Scope *scope, bool has_cond);
void token_while(Allocator *alc, Fc *fc, Scope *scope);
void deref_scope(Allocator *alc, Scope *scope);

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

        if (strcmp(token, "if") == 0) {
            TIf *tif = token_if(alc, fc, scope, true);
            array_push(scope->ast, token_init(alc, tkn_if, tif));
            continue;
        }

        if (strcmp(token, "while") == 0) {
            token_while(alc, fc, scope);
            continue;
        }

        //
        rtok(fc);
        Value *val = read_value(fc, alc, scope, false, 0);

        // Assign
        if (value_assignable(val)) {
            tok(fc, token, false, true);
            sprintf(fc->sbuf, ".%s.", token);
            if (strstr(".=.+=.-=.*=./=.", fc->sbuf)) {
                char *sign = dups(alc, token);
                if (val->type == v_var) {
                    Var *var = val->item;
                    Decl *decl = var->decl;
                    if (!decl->is_mut) {
                        sprintf(fc->sbuf, "Cannot assign value to an immutable variable");
                        fc_error(fc);
                    }
                    if (decl->is_arg && val->rett->class->is_rc) {
                        sprintf(fc->sbuf, "Assigning new values to argument variables is not allowed for performance reasons");
                        fc_error(fc);
                    }
                }
                if (val->type == v_class_pa) {
                    VClassPA *pa = val->item;
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
                    right = value_op(fc, alc, scope, val, right, op_add);
                } else if (strcmp(sign, "-=") == 0) {
                    right = value_op(fc, alc, scope, val, right, op_sub);
                } else if (strcmp(sign, "*=") == 0) {
                    right = value_op(fc, alc, scope, val, right, op_mul);
                } else if (strcmp(sign, "/=") == 0) {
                    right = value_op(fc, alc, scope, val, right, op_div);
                }

                right = try_convert(fc, alc, right, val->rett);
                type_check(fc, val->rett, right->rett);

                upref_value_check(alc, scope, right);

                array_push(scope->ast, tgen_assign(alc, val, right));
                tok_expect(fc, ";", false, true);

                if (val->type == v_var) {
                    Var *var = val->item;
                    Decl *decl = var->decl;
                    Type *type = decl->type;
                    if (type->class && type->class->is_rc) {
                        // New upref slots
                        Scope *scope_ = scope;
                        while (scope_) {
                            UprefSlot *up = map_get(scope_->upref_slots, decl->name);
                            if (up) {
                                map_unset(scope_->upref_slots, decl->name);
                            }
                            if (scope_->type == sct_func) {
                                break;
                            }
                            scope_ = scope_->parent;
                        }
                    }
                }
                continue;
            }
            rtok(fc);
        }

        // Statement
        if (!type_is_void(val->rett)) {
            sprintf(fc->sbuf, "Statement returns a value, but no variable to store it in");
            fc_error(fc);
        }

        array_push(scope->ast, token_init(alc, tkn_statement, val));
        tok_expect(fc, ";", false, true);
    }

    // Derefs
    if (!scope->did_return) {
        deref_scope(alc, scope);
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

    upref_value_check(alc, scope, val);

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

    if (!scope->decls)
        scope->decls = array_make(alc, 8);
    array_push(scope->decls, decl);
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
        Value *tval = try_convert(fc, alc, val, frett);
        type_check(fc, frett, tval->rett);

        upref_value_check(alc, scope, tval);

        TempVar *tvar = al(alc, sizeof(TempVar));
        tvar->value = tval;
        tvar->ir_value = NULL;
        Token *t = token_init(alc, tkn_tmp_var, tvar);
        array_push(scope->ast, t);

        Value *tmp_var = value_init(alc, v_tmp_var, tvar, tval->rett);
        retval = tmp_var;
    }

    deref_scope(alc, scope);

    array_push(scope->ast, tgen_return(alc, fscope, retval));
    tok_expect(fc, ";", false, true);
}

TIf *token_if(Allocator *alc, Fc *fc, Scope *scope, bool has_cond) {
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

    Scope *sub = scope_init(alc, sct_default, scope, true);

    read_ast(fc, sub, single);

    tok(fc, token, false, true);
    TIf *else_if = NULL;
    if (strcmp(token, "else") == 0) {
        tok(fc, token, true, true);
        bool has_if = strcmp(token, "if") == 0;
        if (!has_if) {
            rtok(fc);
        }
        else_if = token_if(alc, fc, scope, has_if);
    } else {
        rtok(fc);
    }

    return tgen_tif(alc, cond, sub, else_if);
}

void token_while(Allocator *alc, Fc *fc, Scope *scope) {
    //
    Value *cond = read_value(fc, alc, scope, true, 0);

    if (!type_is_bool(cond->rett, fc->b)) {
        sprintf(fc->sbuf, "Value must return a bool type");
        fc_error(fc);
    }

    tok_expect(fc, "{", false, true);

    Scope *sub = scope_init(alc, sct_loop, scope, true);
    read_ast(fc, sub, false);

    array_push(scope->ast, tgen_while(alc, cond, sub));
}

void upref_value_check(Allocator *alc, Scope *scope, Value *val) {
    //
    Type *type = val->rett;

    if (type->class && type->class->is_rc) {
        if (val->type == v_var) {
            Var *var = val->item;
            Decl *decl = var->decl;
            decl->times_used++;
            UprefSlot *up = map_get(scope->upref_slots, decl->name);
            if (!up) {
                up = upref_slot_init(alc, decl);
                array_push(scope->ast, token_init(alc, tkn_upref_slot, up));
                map_set(scope->upref_slots, decl->name, up);
            }
            up->count++;
        }
        if (val->type == v_class_pa) {
            // TODO upref token
        }
    }
}

void deref_scope(Allocator *alc, Scope *scope) {
    Array *decls = scope->decls;
    if (!decls)
        return;
    for (int i = 0; i < decls->length; i++) {
        Decl *decl = array_get_index(decls, i);
        Type *type = decl->type;
        Class *class = type->class;
        if (class && decl->times_used != 1) {
            if (class->func_deref) {
                // Call __deref
                Var *var = var_init(alc, decl, type);
                Value *val = value_init(alc, v_var, var, var->type);

                Value *on = vgen_fptr(alc, class->func_deref, NULL);
                Array *values = array_make(alc, 2);
                array_push(values, val);
                array_push(values, vgen_vint(alc, 1, type_gen(class->fc->b, alc, "i32"), false));
                Value *fcall = vgen_fcall(alc, on, values, type_gen_void(alc), scope, false);
                array_push(scope->ast, token_init(alc, tkn_statement, fcall));

            } else if (class->is_rc) {

                Var *var = var_init(alc, decl, type);
                Value *val = value_init(alc, v_var, var, var->type);

                // _RC
                ClassProp *prop = map_get(class->props, "_RC");
                Value *pa = vgen_class_pa(alc, val, prop);

                TempVar *tvar = al(alc, sizeof(TempVar));
                tvar->value = pa;
                tvar->ir_value = NULL;
                Token *tmpt = token_init(alc, tkn_tmp_var, tvar);
                array_push(scope->ast, tmpt);

                Value *tmp_var = value_init(alc, v_tmp_var, tvar, prop->type);

                //
                Value *sub = vgen_op(alc, class->fc->b, tmp_var, vgen_vint(alc, 1, prop->type, false), op_sub, false);

                VPair *pair = al(alc, sizeof(VPair));
                Value *is_zero = vgen_compare(alc, class->fc->b, sub, vgen_vint(alc, 0, prop->type, false), op_eq);

                Scope *code = scope_init(alc, sct_default, scope, true);
                Scope *elif = scope_init(alc, sct_default, scope, true);
                // == 0 : Call free
                Value *on = vgen_fptr(alc, class->func_free, NULL);
                Array *values = array_make(alc, 2);
                array_push(values, val);
                Value *fcall = vgen_fcall(alc, on, values, type_gen_void(alc), code, false);
                array_push(code->ast, token_init(alc, tkn_statement, fcall));

                // != 0 : else update _RC
                Token *as = tgen_assign(alc, pa, sub);
                array_push(elif->ast, as);

                //
                TIf *elift = tgen_tif(alc, NULL, elif, NULL);
                TIf *ift = tgen_tif(alc, is_zero, code, elift);
                Token *t = token_init(alc, tkn_if, ift);
                array_push(scope->ast, t);

                //
                // Var *var = var_init(alc, decl, type);
                // Value *val = value_init(alc, v_var, var, var->type);
                // array_push(scope->ast, token_init(alc, tkn_deref, val));
            }
        }
    }
}
