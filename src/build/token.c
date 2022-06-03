
#include "../all.h"

Token *init_token() {
    Token *t = malloc(sizeof(Token));
    t->type = tkn_unknown;
    t->item = NULL;
    return t;
}

void free_token(Token *token) {
    //
    free(token);
}

void token_return(FileCompiler *fc, Scope *scope) {
    //
    Token *t = init_token();
    t->type = tkn_return;

    scope->did_return = true;

    Scope *vscope = scope;
    while (vscope && vscope->vscope_vname == NULL) {
        vscope = vscope->parent;
    }

    if (vscope) {
        t->type = tkn_set_vscope_value;

        Value *value = fc_read_value(fc, scope, false, true, true);
        Type *rt = vscope->vscope_return_type;
        if (rt == NULL) {
            vscope->vscope_return_type = value->return_type;
        } else if (rt->type == type_null) {
            fc_type_make_nullable(fc, value->return_type);
            vscope->vscope_return_type = value->return_type;
        } else if (value->return_type->type == type_null) {
            fc_type_make_nullable(fc, rt);
        } else {
            fc_type_compatible(fc, rt, value->return_type);
        }

        TokenSetVscopeValue *vt = malloc(sizeof(TokenSetVscopeValue));
        vt->vname = vscope->vscope_vname;
        vt->value = value;
        vt->vscope = vscope;

        t->item = vt;

        fc_expect_token(fc, ";", false, true, true);

        array_push(scope->ast, t);
        return;
    }

    // Check func scope
    Scope *func_scope = scope;
    while (func_scope && func_scope->is_func == false) {
        func_scope = func_scope->parent;
    }

    if (!func_scope || !func_scope->is_func) {
        fc_error(fc, "Trying to return in a non function scope", NULL);
    }

    char *token = malloc(KI_TOKEN_MAX);
    fc_next_token(fc, token, true, true, true);

    if (strcmp(token, ";") != 0) {
        Value *value = fc_read_value(fc, scope, false, true, true);

        if (func_scope->func->return_type == NULL) {
            fc_error(fc, "Function has no return type, but you are returning a value.", NULL);
        }

        fc_type_compatible(fc, func_scope->func->return_type, value->return_type);

        t->item = value;
    }

    fc_expect_token(fc, ";", false, true, true);

    free(token);

    array_push(scope->ast, t);
}

TokenIf *token_if(FileCompiler *fc, Scope *scope, bool is_else, bool has_condition) {
    //
    Value *value = NULL;
    //
    if (has_condition) {
        fc_expect_token(fc, "(", false, true, true);

        value = fc_read_value(fc, scope, false, true, true);
        if (value->return_type->type != type_bool) {
            fc_error(fc, "if statement must return a bool", NULL);
        }

        fc_expect_token(fc, ")", false, true, true);
    }

    fc_expect_token(fc, "{", false, true, true);

    TokenIf *ift = malloc(sizeof(TokenIf));
    ift->condition = value;
    ift->scope = init_sub_scope(scope);
    ift->scope->body_i = fc->i;
    ift->is_else = is_else;
    ift->next = NULL;

    fc_build_ast(fc, ift->scope);

    char *token = malloc(KI_TOKEN_MAX);
    fc_next_token(fc, token, true, false, true);

    if (strcmp(token, "else") == 0) {
        fc_next_token(fc, token, false, false, true);
        bool else_has_cond = false;
        fc_next_token(fc, token, true, true, true);
        if (strcmp(token, "if") == 0) {
            fc_next_token(fc, token, false, true, true);
            else_has_cond = true;
        }
        ift->next = token_if(fc, scope, true, else_has_cond);
    }

    free(token);

    return ift;
}

void token_ifnull(FileCompiler *fc, Scope *scope) {
    //
    char *token = malloc(KI_TOKEN_MAX);
    fc_next_token(fc, token, false, true, true);

    Identifier *id = init_id();
    id->name = strdup(token);
    IdentifierFor *idf = idf_find_in_scope(scope, id);
    if (!idf || idf->type != idfor_local_var) {
        fc_error(fc, "Only local variables are allowed, found: '%s'", token);
    }

    LocalVar *lv = idf->item;

    TokenIfNull *ifn = malloc(sizeof(TokenIfNull));
    ifn->type = or_none;
    ifn->name = lv->gen_name;
    ifn->idf = idf;
    ifn->ort = NULL;

    Token *t = init_token();
    t->type = tkn_ifnull;
    t->item = ifn;

    Type *type = lv->type;

    if (!type->nullable) {
        fc_error(fc, "Using ifnull on variable that doesnt have a nullable type", NULL);
    }
    //
    OrToken *ort = fc_read_or_token(fc, scope, type, token, false);
    ifn->ort = ort;

    if (ort->vscope) {
        // type check scope
        if (ort->type == or_value) {
            // or value
            if (ort->vscope->vscope_return_type->nullable) {
                fc_error(fc, "The set-scope return type cannot be nullable", NULL);
            }
            fc_type_compatible(fc, type, ort->vscope->vscope_return_type);
        } else {
            // or return
            // ast builder does the type checking
        }
    } else if (ort->value) {
        if (ort->type == or_value) {
            // or value
            fc_type_compatible(fc, type, ort->value->return_type);
            if (ort->value->return_type->nullable) {
                fc_error(fc, "The 'set' value cannot be nullable", NULL);
            }
        } else {
            // or return
            Scope *func_scope = get_func_scope(scope);
            if (func_scope->func->return_type) {
                fc_type_compatible(fc, func_scope->func->return_type, ort->value->return_type);
            }
        }
    }

    // Create new type within scope

    Type *ntype = init_type();
    *ntype = *type;
    ntype->nullable = false;

    // local idf
    idf = map_get(scope->identifiers, lv->name);
    LocalVar *nlv = lv;
    if (!idf) {
        // create local identifier
        nlv = malloc(sizeof(LocalVar));
        *nlv = *lv;

        idf = init_idf();
        idf->type = idfor_local_var;
        idf->item = nlv;
        map_set(scope->identifiers, lv->name, idf);
    }

    nlv->type = ntype;

    if (!ort->vscope) {
        fc_expect_token(fc, ";", false, true, true);
    }

    array_push(scope->ast, t);
    //
    free(token);
    free_id(id);
}

char *fc_or_token_err_name(FileCompiler *fc, char *token) {
    //
    char *result = NULL;

    fc_next_token(fc, token, true, true, true);
    if (strcmp(token, "|") == 0) {
        // Value scope

        fc_next_token(fc, token, false, true, true);
        fc_next_token(fc, token, false, true, true);

        if (!is_valid_varname(token)) {
            fc_error(fc, "Invalid variable name for the error", NULL);
        }

        result = strdup(token);

        fc_expect_token(fc, "|", false, false, true);
    }

    return result;
}

OrToken *fc_read_or_token(FileCompiler *fc, Scope *scope, Type *primary_type, char *token, bool on_func_call) {

    OrToken *ort = malloc(sizeof(OrToken));
    ort->type = or_none;
    ort->vscope = NULL;
    ort->value = NULL;
    ort->error = NULL;
    ort->primary_type = primary_type;
    ort->error_vn = NULL;

    fc_next_token(fc, token, false, true, true);
    if (!on_func_call && strcmp(token, "set") == 0) {
        ort->type = or_value;
    } else if (on_func_call && strcmp(token, "value") == 0) {
        ort->error_vn = fc_or_token_err_name(fc, token);
        ort->type = or_value;
        if (!primary_type) {
            fc_error(fc, "Left side has no type, did not expect a value here", NULL);
        }
    } else if (strcmp(token, "return") == 0) {
        if (on_func_call) {
            ort->error_vn = fc_or_token_err_name(fc, token);
        }
        ort->type = or_return;
        Scope *func_scope = get_func_scope(scope);
        ort->primary_type = func_scope->func->return_type;
    } else if (strcmp(token, "throw") == 0) {
        ort->type = or_throw;
        ort->error = fc_read_error_token(fc, err_throw, token);
    } else if (strcmp(token, "panic") == 0) {
        ort->type = or_panic;
        ort->error = fc_read_error_token(fc, err_panic, token);
    } else if (strcmp(token, "exit") == 0) {
        ort->type = or_exit;
        ort->error = fc_read_error_token(fc, err_exit, token);
    } else if (strcmp(token, "break") == 0) {
        ort->type = or_break;
        Scope *loop_scope = get_loop_scope(scope);
        if (!loop_scope) {
            fc_error(fc, "Using break without being inside a loop", NULL);
        }
    } else if (strcmp(token, "continue") == 0) {
        ort->type = or_continue;
        Scope *loop_scope = get_loop_scope(scope);
        if (!loop_scope) {
            fc_error(fc, "Using break without being inside a loop", NULL);
        }
    } else if (on_func_call && strcmp(token, "pass") == 0) {
        ort->type = or_pass;
    } else {
        if (on_func_call) {
            fc_error(fc, "Expected 'value|return|throw|pass|continue|break|panic|exit' but found '%s'", token);
        } else {
            fc_error(fc, "Expected 'set|return|throw|continue|break|panic|exit' but found '%s'", token);
        }
    }

    if (ort->primary_type && (ort->type == or_value || ort->type == or_return)) {

        if (ort->error_vn != NULL) {
            fc_expect_token(fc, "{", true, true, true);
        }

        fc_next_token(fc, token, true, true, true);
        if (strcmp(token, "{") == 0) {
            fc_next_token(fc, token, false, true, true);

            GEN_C++;
            char *vname = malloc(64);
            sprintf(vname, "_KI_VSCOPE_VN_%d", GEN_C);

            Scope *vscope = init_sub_scope(scope);
            vscope->body_i = fc->i;
            if (ort->type == or_value) {
                vscope->vscope_vname = vname;
            }
            vscope->must_return = true;
            ort->vscope = vscope;

            if (ort->error_vn != NULL) {
                Type *errtype = init_type();
                errtype->type = type_throw_msg;

                IdentifierFor *idf = init_idf();
                idf->type = idfor_local_var;
                idf->item = fc_localvar(fc, ort->error_vn, errtype);

                map_set(vscope->identifiers, ort->error_vn, idf);
            }

            fc_build_ast(fc, ort->vscope);

        } else {
            ort->value = fc_read_value(fc, scope, false, true, true);
        }
    }

    return ort;
}

ErrorToken *fc_read_error_token(FileCompiler *fc, int errtype, char *token) {
    //
    ErrorToken *err = malloc(sizeof(ErrorToken));
    err->type = errtype;

    fc_next_token(fc, token, false, true, true);
    err->msg = strdup(token);

    if (err->type == err_exit) {
        // Number
        if (!is_valid_number(token)) {
            fc_error(fc, "Invalid exit code number: '%s'", token);
        }
    }

    // Trace details
    err->filepath = fc->ki_filepath;
    // Todo: get row & col via fc->i

    return err;
}

void token_notnull(FileCompiler *fc, Scope *scope) {
    //
    char *token = malloc(KI_TOKEN_MAX);
    fc_next_token(fc, token, false, true, true);

    Identifier *id = init_id();
    id->name = strdup(token);
    IdentifierFor *idf = idf_find_in_scope(scope, id);
    if (!idf || idf->type != idfor_local_var) {
        fc_error(fc, "Unknown local variable '%s'", token);
    }

    LocalVar *lv = idf->item;

    TokenNotNull *ifn = malloc(sizeof(TokenNotNull));
    ifn->type = or_none;
    ifn->name = lv->gen_name;
    ifn->scope = NULL;

    Token *t = init_token();
    t->type = tkn_notnull;
    t->item = ifn;

    Type *type = lv->type;

    if (!type->nullable) {
        fc_error(fc, "Using notnull on variable that doesnt have a nullable type", NULL);
    }

    fc_next_token(fc, token, false, true, true);
    if (strcmp(token, "do") == 0) {
        ifn->type = or_do;

        fc_expect_token(fc, "{", false, false, true);

        ifn->scope = init_sub_scope(scope);
        ifn->scope->body_i = fc->i;

        // Create new type within scope
        LocalVar *nlv = malloc(sizeof(LocalVar));
        *nlv = *lv;

        Type *ntype = init_type();
        *ntype = *type;
        type = ntype;
        type->nullable = false;

        nlv->type = type;

        IdentifierFor *idfs = init_idf();
        idfs->type = idfor_local_var;
        idfs->item = nlv;

        map_set(ifn->scope->identifiers, nlv->name, idfs);

        fc_build_ast(fc, ifn->scope);
    } else {
        fc_error(fc, "Expected 'do' but found '%s'", token);
    }

    array_push(scope->ast, t);
    //
    free(token);
    free_id(id);
}

void token_while(FileCompiler *fc, Scope *scope) {
    //
    fc_expect_token(fc, "(", false, true, true);
    Value *value = fc_read_value(fc, scope, false, true, true);

    if (value->return_type->type != type_bool) {
        fc_error(fc, "while condition must return a bool", NULL);
    }

    fc_expect_token(fc, ")", false, true, true);
    fc_expect_token(fc, "{", false, true, true);
    Scope *wscope = init_sub_scope(scope);
    wscope->body_i = fc->i;
    wscope->is_loop = true;
    wscope->in_loop = true;
    Token *t = init_token();
    t->type = tkn_while;
    TokenWhile *wt = malloc(sizeof(TokenWhile));
    wt->condition = value;
    wt->scope = wscope;
    t->item = wt;

    fc_build_ast(fc, wscope);

    array_push(scope->ast, t);
}

void token_throw(FileCompiler *fc, Scope *scope) {
    //
    Token *t = init_token();
    t->type = tkn_throw;
    char *msg = malloc(KI_TOKEN_MAX);
    fc_next_token(fc, msg, false, true, true);

    Scope *throw_scope = scope;
    while (throw_scope && throw_scope->is_func == false) {
        throw_scope = throw_scope->parent;
    }

    if (throw_scope == NULL) {
        fc_error(fc, "Trying to throw inside a non function space", NULL);
    }
    if (throw_scope->func->can_error == false) {
        fc_error(fc, "Trying to throw inside a function that cannot return an error", NULL);
    }

    TokenThrow *tt = malloc(sizeof(TokenThrow));
    tt->msg = msg;
    tt->return_type = throw_scope->func->return_type;

    t->item = tt;
    array_push(scope->ast, t);

    fc_expect_token(fc, ";", false, true, true);

    scope->did_return = true;
}

void token_each(FileCompiler *fc, Scope *scope) {
    //
    Token *t = init_token();
    t->type = tkn_each;

    TokenEach *te = malloc(sizeof(TokenEach));
    te->value = fc_read_value(fc, scope, false, true, true);
    te->kvar = NULL;

    fc_expect_token(fc, "as", false, true, true);

    char *token = malloc(KI_TOKEN_MAX);
    fc_next_token(fc, token, false, true, true);

    if (!is_valid_varname(token)) {
        fc_error(fc, "Invalid item var name: '%s'", token);
    }

    te->vvar = fc_localvar(fc, strdup(token), NULL);
    fc_next_token(fc, token, true, true, true);

    if (is_valid_varname(token)) {
        fc_next_token(fc, token, false, true, true);
        te->kvar = fc_localvar(fc, strdup(token), fc_identifier_to_type(fc, create_identifier("ki", "type", "uxx"), NULL));
    }

    if (!te->value->return_type->class) {
        fc_error(fc, "Invalid each value", NULL);
    }
    Class *class = te->value->return_type->class;
    ClassProp *countf = map_get(class->props, "__each_count");
    ClassProp *getf = map_get(class->props, "__each_get");
    if (!countf || !getf || !countf->is_func || !getf->is_func || getf->is_static || countf->is_static) {
        fc_error(fc, "Invalid each value: Missing __each_count / __each_get non-static functions", NULL);
    }
    if (countf->func->args->length != 1) {
        fc_error(fc, "__each_count has too many arguments", NULL);
    }
    if (getf->func->args->length != 2) {
        fc_error(fc, "__each_get must have 1 argument of type 'uxx'", NULL);
    }
    Type *ret = countf->func->return_type;
    if (!ret || !ret->class || strcmp(ret->class->cname, "ki__type__uxx") != 0 || countf->func->can_error) {
        fc_error(fc, "__each_count must have the return type 'uxx'", NULL);
    }
    ret = getf->func->return_type;
    if (!ret || !getf->func->can_error) {
        fc_error(fc, "__each_get must have a return type and allow errors", NULL);
    }

    te->vvar->type = ret;

    fc_expect_token(fc, "{", false, true, true);

    te->scope = init_sub_scope(scope);
    te->scope->body_i = fc->i;
    te->scope->is_loop = true;
    te->scope->in_loop = true;

    IdentifierFor *idf = init_idf();
    idf->type = idfor_local_var;
    idf->item = te->vvar;

    map_set(te->scope->identifiers, te->vvar->name, idf);

    if (te->kvar) {
        IdentifierFor *idfk = init_idf();
        idfk->type = idfor_local_var;
        idfk->item = te->kvar;
        map_set(te->scope->identifiers, te->kvar->name, idfk);
    }

    fc_build_ast(fc, te->scope);

    t->item = te;
    array_push(scope->ast, t);
    free(token);
}

void token_break(FileCompiler *fc, Scope *scope) {
    //
    Token *t = init_token();
    t->type = tkn_break;
    array_push(scope->ast, t);

    fc_expect_token(fc, ";", false, true, true);
}

void token_continue(FileCompiler *fc, Scope *scope) {
    //
    Token *t = init_token();
    t->type = tkn_continue;
    array_push(scope->ast, t);

    fc_expect_token(fc, ";", false, true, true);
}

void token_free(FileCompiler *fc, Scope *scope) {
    //
    Token *t = init_token();
    t->type = tkn_free;
    t->item = fc_read_value(fc, scope, false, true, true);
    array_push(scope->ast, t);
    fc_expect_token(fc, ";", false, true, true);
}

void token_declare(FileCompiler *fc, Scope *scope, Type *left_type) {
    // Get var name
    char *token = malloc(KI_TOKEN_MAX);
    fc_next_token(fc, token, false, true, true);
    fc_name_taken(fc, scope->identifiers, token);

    //
    fc_expect_token(fc, "=", false, true, true);
    //
    Value *value = fc_read_value(fc, scope, false, true, true);

    if (left_type) {
        if (!type_compatible(left_type, value->return_type)) {
            fc_error(fc, "Types are not compatible", NULL);
        }
    }

    IdentifierFor *idf = init_idf();
    idf->type = idfor_local_var;
    LocalVar *lv = fc_localvar(fc, token, left_type ? left_type : value->return_type);
    idf->item = lv;

    map_set(scope->identifiers, lv->name, idf);

    TokenDeclare *decl = malloc(sizeof(TokenDeclare));
    decl->name = lv->gen_name;
    decl->value = value;
    decl->type = lv->type;

    Token *t = init_token();
    t->type = tkn_declare;
    t->item = decl;

    array_push(scope->ast, t);

    fc_expect_token(fc, ";", false, true, true);
}

void token_assign(FileCompiler *fc, Scope *scope, char *sign, Value *left) {
    //
    if (left->type == vt_arg && left->return_type->class && left->return_type->class->ref_count) {
        fc_error(fc, "Assigning new values to argument variables is not allowed. We disabled this because it reduces performance. Use a separate variable.", NULL);
    }
    //
    TokenAssign *ta = malloc(sizeof(TokenAssign));
    ta->left = left;

    if (strcmp(sign, "=") == 0) {
        ta->type = op_eq;
    } else if (strcmp(sign, "+=") == 0) {
        ta->type = op_add;
    } else if (strcmp(sign, "-=") == 0) {
        ta->type = op_sub;
    } else if (strcmp(sign, "*=") == 0) {
        ta->type = op_mult;
    } else if (strcmp(sign, "/=") == 0) {
        ta->type = op_div;
    } else if (strcmp(sign, "\%=") == 0) {
        ta->type = op_mod;
    } else {
        fc_error(fc, "Invalid assign token: '%s'", sign);
    }

    ta->right = fc_read_value(fc, scope, false, true, true);

    Token *t = init_token();
    t->type = tkn_assign;
    t->item = ta;

    array_push(scope->ast, t);
}