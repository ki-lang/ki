
#include "../all.h"

void fc_mark_accesses_globals(Function *func);

void fc_build_asts() {
    //
    for (int i = 0; i < g_functions->length; i++) {
        Function *func = array_get_index(g_functions, i);

        FileCompiler *fc = func->fc;
        fc->add_use_target = func->cname;

        if (!fc->should_recompile) {
            continue;
        }

        if (!fc->is_header) {
            // printf("ast:%s\n", func->cname);
            fc_build_ast(fc, func->scope);
        }

        Token *t = init_token();
        t->type = tkn_func;
        t->item = func;
        array_push(fc->scope->ast, t);
    }

    // Mark functions that access globals
    for (int i = 0; i < g_functions->length; i++) {
        Function *func = array_get_index(g_functions, i);
        if (func->accesses_globals) {
            fc_mark_accesses_globals(func);
            continue;
        }
    }

    // Scan global values
    for (int i = 0; i < packages->keys->length; i++) {
        PkgCompiler *pkc = array_get_index(packages->values, i);
        for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
            FileCompiler *fc = array_get_index(pkc->file_compilers->values, o);

            if (!fc->should_recompile) {
                continue;
            }

            fc_scan_global_values(fc);
        }
    }
}

void fc_build_ast(FileCompiler *fc, Scope *scope) {
    //
    Scope *prev_fscope = fc->current_func_scope;
    if (scope->is_func) {
        fc->current_func_scope = scope;
    }
    //
    char *token = malloc(KI_TOKEN_MAX);
    fc->i = scope->body_i;

    while (fc->i < fc->content_len) {
        fc_next_token(fc, token, false, false, true);

        if (strcmp(token, "}") == 0) {
            break;
        }

        if (strcmp(token, "#") == 0) {
            fc_parse_macro(fc, scope, token);
            continue;
        }

        if (strcmp(token, "each") == 0) {
            token_each(fc, scope);
            continue;
        }

        if (strcmp(token, "DEBUGMSG") == 0) {
            Token *tk = init_token();
            tk->type = tkn_debug_msg;
            fc_next_token(fc, token, false, true, true);
            tk->item = strdup(token);
            array_push(scope->ast, tk);
            fc_expect_token(fc, ";", false, true, true);
            continue;
        }

        if (strcmp(token, "exit") == 0) {
            Token *tk = init_token();
            tk->type = tkn_exit;
            tk->item = fc_read_error_token(fc, err_exit, token);
            array_push(scope->ast, tk);
            fc_expect_token(fc, ";", false, true, true);
            continue;
        }

        if (strcmp(token, "panic") == 0) {
            Token *tk = init_token();
            tk->type = tkn_panic;
            tk->item = fc_read_error_token(fc, err_panic, token);
            array_push(scope->ast, tk);
            fc_expect_token(fc, ";", false, true, true);
            continue;
        }

        if (strcmp(token, "return") == 0) {
            token_return(fc, scope);
            continue;
        }

        if (strcmp(token, "if") == 0) {
            TokenIf *ift = token_if(fc, scope, false, true);
            Token *tk = init_token();
            tk->type = tkn_if;
            tk->item = ift;
            array_push(scope->ast, tk);
            continue;
        }

        if (strcmp(token, "ifnull") == 0) {
            token_ifnull(fc, scope);
            continue;
        }
        if (strcmp(token, "notnull") == 0) {
            token_notnull(fc, scope);
            continue;
        }

        if (strcmp(token, "while") == 0) {
            token_while(fc, scope);
            continue;
        }

        if (strcmp(token, "throw") == 0) {
            token_throw(fc, scope);
            continue;
        }

        if (scope->in_loop) {
            if (strcmp(token, "break") == 0) {
                token_break(fc, scope);
                continue;
            }

            if (strcmp(token, "continue") == 0) {
                token_continue(fc, scope);
                continue;
            }
        }

        //
        if (strcmp(token, "@") == 0) {
            token_declare(fc, scope, NULL);
            continue;
        }

        if (strcmp(token, "?") == 0) {
            fc->i--;
            Type *type = fc_read_type(fc, scope);
            token_declare(fc, scope, type);
            continue;
        }

        // Identifier
        fc->i -= strlen(token);
        int current_i = fc->i;
        IdentifierFor *idf = fc_read_and_get_idf(fc, scope, false, true, true);

        // If generic read/skip generic types
        if (idf && idf->type == idfor_class) {
            Class *class = idf->item;
            if (class->generic_names != NULL && class->generic_hash == NULL) {
                Class *gclass = fc_get_generic_class(fc, class, scope);
            }
        }

        // Check if declare
        if (idf && (idf->type == idfor_class || idf->type == idfor_enum || idf->type == idfor_type) && fc_get_char(fc, 0) == ' ') {
            fc_next_token(fc, token, true, true, true);
            if (is_valid_varname(token)) {
                // Var declaration
                fc->i = current_i;
                Type *type = fc_read_type(fc, scope);
                token_declare(fc, scope, type);
                continue;
            }
        }

        // Value
        fc->i = current_i;
        Value *value = fc_read_value(fc, scope, false, true, true);

        // Check if assign
        fc_next_token(fc, token, true, true, true);
        if ((value->type == vt_arg || value->type == vt_var || value->type == vt_shared_global || value->type == vt_threaded_global || value->type == vt_prop_access) && (strcmp(token, "=") == 0 || strcmp(token, "-=") == 0 || strcmp(token, "+=") == 0 || strcmp(token, "*=") == 0 || strcmp(token, "/=") == 0 || strcmp(token, "\%=") == 0)) {
            //
            if (value->type == vt_prop_access) {
                ValueClassPropAccess *pa = value->item;
                if (pa->is_static || value->return_type->type == type_funcref) {
                    fc_error(fc, "Cannot assign a value to this property", NULL);
                }
                // Check if prop is public
                Value *class_instance_value = pa->on;
                Class *class = class_instance_value->return_type->class;
                Scope *class_scope = get_class_scope(scope);
                if (!class_scope || class_scope->class != class) {
                    ClassProp *prop = map_get(class->props, pa->name);
                    if (prop->access_type != acct_public) {
                        fc_error(fc, "Cannot assign a value to a private or readonly property", NULL);
                    }
                }
            }

            fc_next_token(fc, token, false, true, true);
            token_assign(fc, scope, token, value);
            fc_expect_token(fc, ";", false, true, true);
            continue;
        }

        // Plain value
        Token *t = init_token();
        t->type = tkn_value;
        t->item = value;

        if (value->return_type != NULL && value->return_type->type != type_void) {
            fc_error(fc,
                     "Statement that returns a value, but has no variable to store "
                     "it in",
                     NULL);
        }

        array_push(scope->ast, t);

        fc_expect_token(fc, ";", false, true, true);

        continue;
        // fc_error(fc, "Unexpected token: '%s'\n", token);
    }

    if (scope->must_return && !scope->did_return) {
        fc_error(fc, "Scope is missing a return statement.", NULL);
    }

    free(token);

    //
    fc->current_func_scope = prev_fscope;
}

void fc_mark_accesses_globals(Function *func) {
    //
    for (int i = 0; i < func->called_by->length; i++) {
        Function *by = array_get_index(func->called_by, i);
        if (by->accesses_globals) {
            continue;
        }
        by->accesses_globals = true;
        fc_mark_accesses_globals(by);
    }
}
