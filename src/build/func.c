
#include "../all.h"

Function *init_func() {
    Function *func = malloc(sizeof(Function));
    func->cname = NULL;
    func->hash = NULL;
    func->fc = NULL;
    func->can_error = false;
    func->generate_code = true;
    func->is_test = false;
    func->accesses_globals = false;
    func->args = array_make(2);
    func->arg_types = array_make(2);
    func->args_i = 0;
    func->args_i_end = 0;
    func->scope = NULL;
    func->return_type = NULL;
    func->called_by = array_make(2);
    func->class = NULL;
    return func;
}

void free_func(Function *func) {
    //
    free(func);
}

FunctionArg *init_func_arg() {
    FunctionArg *arg = malloc(sizeof(Function));
    arg->name = NULL;
    arg->type = NULL;
    arg->default_value = NULL;
    return arg;
}

void free_func_arg(FunctionArg *arg) {
    //
    free(arg);
}

void fc_scan_func(FileCompiler *fc, Function *func) {
    char *token = malloc(KI_TOKEN_MAX);
    // Args
    fc_expect_token(fc, "(", false, true, true);
    func->args_i = fc->i;
    fc_skip_body(fc, "(", ")", NULL, false);
    func->args_i_end = fc->i;
    // Return type
    fc_next_token(fc, token, true, false, true);
    if (strcmp(token, "!") == 0) {
        fc_next_token(fc, token, false, false, true);
        fc_next_token(fc, token, true, false, true);
    }
    if (fc->is_header) {
        // Header
        if (strcmp(token, ";") != 0) {
            fc_skip_type(fc);
        }
        fc_expect_token(fc, ";", false, true, true);
    } else {
        // Not header
        if (strcmp(token, "{") != 0) {
            fc_skip_type(fc);
        }
        // Body
        fc_expect_token(fc, "{", false, true, true);
        func->scope->body_i = fc->i;
        fc_skip_body(fc, "{", "}", NULL, false);
        func->scope->body_i_end = fc->i;
    }

    free(token);
}

void fc_scan_func_args(Function *func) {
    //
    char *token = malloc(KI_TOKEN_MAX);
    FileCompiler *fc = func->fc;
    Scope *scope = func->scope;
    fc->i = func->args_i;
    Array *names = array_make(10);
    //
    while (fc->i < func->args_i_end) {
        fc_next_token(fc, token, true, true, true);

        if (strcmp(token, ")") == 0) {
            fc_next_token(fc, token, false, true, true);
            break;
        }

        Type *type = fc_read_type(fc, func->scope->parent);

        fc_next_token(fc, token, false, false, true);
        if (!is_valid_varname(token)) {
            fc_error(fc, "Invalid arg name: '%s'\n", token);
        }
        FunctionArg *prev = map_get(scope->identifiers, token);
        if (prev) {
            fc_error(fc, "Duplicate argument name: '%s'", token);
        }

        char *name = strdup(token);
        int find = array_find(names, name, "chars");
        if (find > -1) {
            fc_error(fc, "Name already used in previous argument: '%s'\n", name);
        }

        FunctionArg *arg = init_func_arg();
        arg->name = name;
        arg->type = type;

        array_push(func->args, arg);
        array_push(func->arg_types, arg->type);

        LocalVar *lv = malloc(sizeof(LocalVar));
        lv->name = name;
        lv->gen_name = name;
        lv->type = type;

        IdentifierFor *idf = init_idf();
        idf->type = idfor_arg;
        idf->item = lv;

        map_set(scope->identifiers, name, idf);

        // , or )
        fc_next_token(fc, token, false, false, true);

        if (strcmp(token, ")") == 0) {
            break;
        }
    }
    array_free(names);

    if (strcmp(func->cname, "main") == 0) {
        if (func->args->length > 1) {
            fc_error(fc, "Too many arguments for main", NULL);
        }
        if (func->args->length == 1) {
            FunctionArg *arg = array_get_index(func->args, 0);
            Type *type = fc_identifier_to_type(fc, create_identifier("ki", "type", "Array"), NULL);
            Type *subtype = fc_identifier_to_type(fc, create_identifier("ki", "type", "String"), NULL);
            Type *gent = type_generate_generic(type, subtype);
            if (!type_compatible(gent, arg->type)) {
                fc_error(fc, "First argument of 'main' must of type Array<String>", NULL);
            }
        }
    }

    // Return type
    fc_next_token(fc, token, true, false, true);
    if (strcmp(token, fc->is_header ? ";" : "{") != 0) {
        if (strcmp(token, "!") == 0) {
            fc_next_token(fc, token, false, false, true); // skip '!'
            fc_next_token(fc, token, true, false, true);  // read next
            func->can_error = true;
        }
        if (strcmp(token, fc->is_header ? ";" : "{") != 0) {
            Type *return_type = fc_read_type(fc, func->scope->parent);
            func->return_type = return_type;
            func->scope->must_return = true;
        }
    }

    // Check main return type
    if (strcmp(func->cname, "main") == 0) {
        if (func->can_error) {
            fc_error(fc, "Function 'main' is not allowed to throw errors", NULL);
        }
        Type *ret_type = func->return_type;
        Type *ret_expect = fc_identifier_to_type(fc, create_identifier("ki", "type", "i32"), NULL);
        if (!type_compatible(ret_expect, ret_type)) {
            fc_error(fc, "Function 'main' must have return type 'i32'", NULL);
        }
    }

    //
    if (!fc->is_header) {
        fc_expect_token(fc, "{", false, false, true);
        func->scope->body_i = fc->i;
    }

    //
    free(token);
}
