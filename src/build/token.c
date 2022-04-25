
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

    // Check value scope
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

        t->item = vt;

        fc_expect_token(fc, ";", false, true, true);

        scope->did_return = true;

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

    scope->did_return = true;

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
    if (!idf || idf->type != idfor_var) {
        fc_error(fc, "Unknown variable '%s'", token);
    }

    TokenIfNull *ifn = malloc(sizeof(TokenIfNull));
    ifn->type = or_none;
    ifn->name = strdup(id->name);
    ifn->value = NULL;
    ifn->then_scope = NULL;
    ifn->throw_msg = NULL;
    ifn->return_scope = NULL;

    Token *t = init_token();
    t->type = tkn_ifnull;
    t->item = ifn;

    Type *type = idf->item;

    if (!type->nullable) {
        fc_error(fc, "Using ifnull on variable that doesnt have a nullable type", NULL);
    }
    //
    fc_next_token(fc, token, false, true, true);
    if (strcmp(token, "set") == 0) {
        ifn->type = or_value;
        ifn->value = fc_read_value(fc, scope, false, true, true);
        // Type check
        fc_type_compatible(fc, type, ifn->value->return_type);
        if (ifn->value->return_type->nullable) {
            fc_error(fc, "The 'set' value cannot be nullable", NULL);
        }
    } else if (strcmp(token, "throw") == 0) {
        ifn->type = or_throw;
        fc_next_token(fc, token, false, true, true);
        ifn->throw_msg = strdup(token);
    } else if (strcmp(token, "return") == 0) {
        ifn->type = or_return;
        ifn->value = fc_read_value(fc, scope, false, true, true);
        Scope *func_scope = get_func_scope(scope);
        fc_type_compatible(fc, func_scope->func->return_type, ifn->value->return_type);
        ifn->return_scope = init_sub_scope(scope);
    } else {
        fc_error(fc, "Expected 'set, throw or return' but found '%s'", token);
    }

    // Create new type within scope
    Type *ntype = init_type();
    *ntype = *type;
    ntype->nullable = false;
    idf->item = ntype;

    // then { ... }
    fc_next_token(fc, token, true, true, true);
    if (ifn->type == or_value && strcmp(token, "then") == 0) {

        fc_next_token(fc, token, false, true, true);
        fc_expect_token(fc, "{", false, false, true);

        ifn->then_scope = init_sub_scope(scope);
        ifn->then_scope->body_i = fc->i;

        fc_build_ast(fc, ifn->then_scope);
    } else {
        fc_expect_token(fc, ";", false, true, true);
    }

    array_push(scope->ast, t);
    //
    free(token);
    free_id(id);
}

void token_notnull(FileCompiler *fc, Scope *scope) {
    //
    char *token = malloc(KI_TOKEN_MAX);
    fc_next_token(fc, token, false, true, true);

    Identifier *id = init_id();
    id->name = strdup(token);
    IdentifierFor *idf = idf_find_in_scope(scope, id);
    if (!idf || idf->type != idfor_var) {
        fc_error(fc, "Unknown variable '%s'", token);
    }

    TokenNotNull *ifn = malloc(sizeof(TokenNotNull));
    ifn->type = or_none;
    ifn->name = strdup(id->name);
    ifn->scope = NULL;

    Token *t = init_token();
    t->type = tkn_notnull;
    t->item = ifn;

    Type *type = idf->item;

    if (!type->nullable) {
        fc_error(fc, "Using ifnull on variable that doesnt have a nullable type", NULL);
    }

    fc_next_token(fc, token, false, true, true);
    if (strcmp(token, "do") == 0) {
        ifn->type = or_do;

        fc_expect_token(fc, "{", false, false, true);

        ifn->scope = init_sub_scope(scope);
        ifn->scope->body_i = fc->i;

        // Create new type within scope
        Type *ntype = init_type();
        *ntype = *type;
        type = ntype;
        type->nullable = false;

        IdentifierFor *idfs = init_idf();
        idfs->type = idfor_var;
        idfs->item = ntype;

        map_set(ifn->scope->identifiers, ifn->name, idfs);

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
    te->kname = NULL;

    fc_expect_token(fc, "as", false, true, true);

    char *token = malloc(KI_TOKEN_MAX);
    fc_next_token(fc, token, false, true, true);

    if (!is_valid_varname(token)) {
        fc_error(fc, "Invalid item var name: '%s'", token);
    }

    te->vname = strdup(token);
    fc_next_token(fc, token, true, true, true);

    if (is_valid_varname(token)) {
        fc_next_token(fc, token, false, true, true);
        te->kname = strdup(token);
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

    fc_expect_token(fc, "{", false, true, true);

    te->scope = init_sub_scope(scope);
    te->scope->body_i = fc->i;
    te->scope->is_loop = true;
    te->scope->in_loop = true;

    IdentifierFor *idf = init_idf();
    idf->type = idfor_var;
    idf->item = ret;
    map_set(te->scope->identifiers, te->vname, idf);

    if (te->kname) {
        IdentifierFor *idfk = init_idf();
        idfk->type = idfor_var;
        idfk->item = fc_identifier_to_type(fc, create_identifier("ki", "type", "uxx"), NULL);
        map_set(te->scope->identifiers, te->kname, idfk);
    }

    fc_build_ast(fc, te->scope);

    t->item = te;
    array_push(scope->ast, t);
    free(token);
}

void token_static(FileCompiler *fc, Scope *scope) {
    // Get type
    Type *left_type = fc_read_type(fc, scope);
    if (left_type == NULL) {
        fc_error(fc, "Missing type", NULL);
    }

    // Get var name
    char *token = malloc(KI_TOKEN_MAX);
    fc_next_token(fc, token, false, true, true);
    fc_name_taken(fc, scope->identifiers, token);

    //
    fc_expect_token(fc, "{", false, true, true);
    //
    Function *func = init_func();
    func->fc = fc;
    func->return_type = left_type;
    func->scope = init_sub_scope(fc->scope);
    func->scope->is_func = true;
    func->scope->func = func;
    func->scope->body_i = fc->i;
    func->scope->is_func = true;

    char *gname = malloc(128);
    sprintf(gname, "_KI_STATIC_%s_%d", fc->hash, fc->static_vars->length);
    char *fn = malloc(128);
    sprintf(fn, "_KI_STATIC_%s_%d_init", fc->hash, fc->var_bufc);
    func->cname = fn;

    array_push(fc->functions, func);

    fc_build_ast(fc, func->scope);

    if (!left_type && !scope->did_return) {
        fc_error(fc, "Scope must return a value", NULL);
    }

    TokenStaticDeclare *decl = malloc(sizeof(TokenStaticDeclare));
    decl->name = token;
    decl->global_name = gname;
    decl->scope = func->scope;
    decl->type = left_type;

    Token *t = init_token();
    t->type = tkn_static;
    t->item = decl;

    array_push(scope->ast, t);
    array_push(fc->static_vars, decl);

    IdentifierFor *idf = init_idf();
    idf->type = idfor_static_var;
    idf->item = decl;

    map_set(scope->identifiers, decl->name, idf);
}

void token_set_threaded(FileCompiler *fc, Scope *scope) {
    Token *t = init_token();
    t->type = tkn_set_threaded;

    Identifier *id = fc_read_identifier(fc, false, true, true);
    Scope *idf_scope = fc_get_identifier_scope(fc, fc->scope, id);
    IdentifierFor *idf = idf_find_in_scope(idf_scope, id);

    if (!idf || idf->type != idfor_threaded_var) {
        fc_error(fc, "Cannot find threaded global variable: %s", id->name);
    }

    ThreadedGlobal *tg = idf->item;

    fc_expect_token(fc, "=", false, true, true);

    TokenIdValue *iv = malloc(sizeof(TokenIdValue));
    iv->name = fc_create_identifier_global_cname(fc, id);
    iv->value = fc_read_value(fc, scope, false, true, true);

    // Todo: type check value->return_type with tg->type

    t->item = iv;
    array_push(scope->ast, t);

    fc_expect_token(fc, ";", false, true, true);
}

void token_mutex_init(FileCompiler *fc, Scope *scope) {
    Value *val = fc_read_value(fc, scope, false, true, true);

    Token *t = init_token();
    t->type = tkn_mutex_init;
    t->item = val;

    array_push(scope->ast, t);
    fc_expect_token(fc, ";", false, true, true);
}

void token_mutex_lock(FileCompiler *fc, Scope *scope) {
    Value *val = fc_read_value(fc, scope, false, true, true);

    Token *t = init_token();
    t->type = tkn_mutex_lock;
    t->item = val;

    array_push(scope->ast, t);
    fc_expect_token(fc, ";", false, true, true);
}

void token_mutex_unlock(FileCompiler *fc, Scope *scope) {
    Value *val = fc_read_value(fc, scope, false, true, true);

    Token *t = init_token();
    t->type = tkn_mutex_unlock;
    t->item = val;

    array_push(scope->ast, t);
    fc_expect_token(fc, ";", false, true, true);
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

    TokenDeclare *decl = malloc(sizeof(TokenDeclare));
    decl->name = token;
    decl->value = value;
    decl->type = left_type ? left_type : value->return_type;

    Token *t = init_token();
    t->type = tkn_declare;
    t->item = decl;

    array_push(scope->ast, t);

    IdentifierFor *idf = init_idf();
    idf->type = idfor_var;
    idf->item = decl->type;

    map_set(scope->identifiers, decl->name, idf);

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