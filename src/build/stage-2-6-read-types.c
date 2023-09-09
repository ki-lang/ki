

#include "../all.h"

void stage_2_6_class_default_functions(Fc *fc, Class *class);

void stage_2_6(Fc *fc) {
    // Aliasses
    Build *b = fc->b;
    if (b->verbose > 2) {
        printf("# Stage 2.1 : Read aliasses : %s\n", fc->path_ki);
    }

    for (int i = 0; i < fc->funcs->length; i++) {
        Func *func = array_get_index(fc->funcs, i);
        if (!func->chunk_args)
            continue;
        if (b->verbose > 2) {
            printf("> Scan func types: %s\n", func->dname);
        }
        stage_2_6_func(fc, func);
    }
    for (int i = 0; i < fc->globals->length; i++) {
        Global *g = array_get_index(fc->globals, i);
        fc->chunk = g->type_chunk;
        Type *type = read_type(fc, fc->alc, fc->scope, true, true, rtc_default);
        if (type->ptr_depth > 0 && !type->nullable) {
            sprintf(fc->sbuf, "Global types must be prefixed with '?' because the default value of a global is 'null' for pointer types");
            fc_error(fc);
        }
        g->type = type;
    }
    for (int i = 0; i < fc->classes->length; i++) {
        Class *class = array_get_index(fc->classes, i);
        if (class->is_generic_base)
            continue;
        stage_2_6_class_default_functions(fc, class);
    }

    chain_add(b->stage_3, fc);
}

void stage_2_6_class_functions(Fc *fc, Class *class) {
    //
    if (class->is_generic_base)
        return;

    Array *funcs = class->funcs->values;
    for (int i = 0; i < funcs->length; i++) {
        Func *func = array_get_index(funcs, i);
        stage_2_6_func(fc, func);
    }

    stage_2_6_class_default_functions(fc, class);
}

void stage_2_6_func(Fc *fc, Func *func) {
    //
    Func *prev_error_func_info = fc->error_func_info;
    fc->error_func_info = func;
    //
    char *token = fc->token;
    Allocator *alc = fc->alc;

    // Args
    fc->chunk = func->chunk_args;
    if (func->parse_args) {
        tok(fc, token, true, true);
        while (strcmp(token, ")") != 0) {

            if (!is_valid_varname(token)) {
                sprintf(fc->sbuf, "Invalid argument name: '%s'", token);
                fc_error(fc);
            }
            if (map_get(func->args_by_name, token)) {
                sprintf(fc->sbuf, "Argument name already used: '%s'", token);
                fc_error(fc);
            }

            char *name = dups(alc, token);

            tok_expect(fc, ":", true, true);

            Chunk *val_chunk = NULL;
            Chunk *type_chunk = chunk_clone(alc, fc->chunk);

            Type *type = read_type(fc, alc, func->scope->parent, true, true, rtc_func_arg);

            tok(fc, token, true, true);
            if (strcmp(token, "=") == 0) {
                val_chunk = chunk_clone(alc, fc->chunk);
                skip_value(fc);
            } else {
                rtok(fc);
            }

            tok(fc, token, false, true);
            if (strcmp(token, ",") == 0) {
                tok(fc, token, false, true);
            } else if (strcmp(token, ")") != 0) {
                sprintf(fc->sbuf, "Unexpected token '%s'", token);
                fc_error(fc);
            }

            Arg *arg = arg_init(alc, name, type);
            arg->value_chunk = val_chunk;
            arg->value_chunk_scope = func->scope;
            arg->type_chunk = type_chunk;

            array_push(func->args, arg);
            map_set(func->args_by_name, name, arg);
        }
    }

    // Return type
    tok(fc, token, false, true);
    if (strcmp(token, "!") != 0 && strcmp(token, "%") != 0 && strcmp(token, "{") != 0) {
        rtok(fc);
        func->rett = read_type(fc, alc, func->scope->parent, true, true, rtc_func_rett);
        tok(fc, token, false, true);
    }

    if (func->will_exit && !type_is_void(func->rett)) {
        sprintf(fc->sbuf, "Using '!' before the function name tells the compiler this function will exit the program. Therefore the return type must be void.");
        fc_error(fc);
    }

    Array *errors = NULL;

    while (strcmp(token, "!") == 0) {
        if (!errors) {
            errors = array_make(alc, 4);
            func->can_error = true;
        }

        tok(fc, token, true, false);
        if (!is_valid_varname(token)) {
            sprintf(fc->sbuf, "Invalid error name '%s'", token);
            fc_error(fc);
        }
        if (array_contains(errors, token, arr_find_str)) {
            sprintf(fc->sbuf, "Duplicate error name '%s'", token);
            fc_error(fc);
        }
        array_push(errors, dups(alc, token));

        tok(fc, token, false, true);
    }

    func->errors = errors;

    while (strcmp(token, "%") == 0) {
        tok(fc, token, false, false);
        if (strcmp(token, "hot") == 0) {
            func->opt_hot = true;
        } else if (strcmp(token, "inline") == 0) {
            func->opt_inline = true;
        } else {
            sprintf(fc->sbuf, "Unknown tag '%s'", token);
            fc_error(fc);
        }

        tok(fc, token, false, true);
    }

    // Define arguments in AST
    func_make_arg_decls(func);

    rtok(fc);

    Build *b = fc->b;
    if (func == b->main_func) {
        // Type check arguments (TODO)

        // Type check return type
        Type *rett = func->rett;
        Class *class = rett->class;
        if ((!type_is_void(rett) && class != ki_get_class(b, "type", "i32")) || rett->ptr_depth > 0) {
            sprintf(fc->sbuf, "func 'main' should return 'void' or 'i32'");
            fc_error(fc);
        }
    }

    fc->error_func_info = prev_error_func_info;
}

void stage_2_6_class_default_functions(Fc *fc, Class *class) {

    // Generate __free / __deref / _RC
    if (class->type == ct_struct) {
        Build *b = fc->b;
        if (class->is_rc) {
            // Define __deref_props
            // Only generate the function if there are properties that can be dereferenced
            Array *props = class->props->values;
            for (int i = 0; i < props->length; i++) {
                ClassProp *prop = array_get_index(props, i);
                Class *pclass = prop->type->class;
                if (pclass && pclass->is_rc) {
                    class->func_deref_props = class_define_func(fc, class, false, "__deref_props", NULL, b->type_void, 0);
                    class->func_deref_props->is_generated = true;
                    break;
                }
            }
        }
        // Define __free
        if (!class->func_free) {
            class->func_free = class_define_func(fc, class, false, "__free", NULL, b->type_void, 0);
            class->func_free->is_generated = true;
        }
    }

    if (class->is_rc) {
        if (class->type != ct_struct) {
            if (!class->func_ref) {
                sprintf(fc->sbuf, "Missing function '__ref' in type '%s'", class->dname);
                fc_error(fc);
            }
            if (!class->func_deref) {
                sprintf(fc->sbuf, "Missing function '__deref' in type '%s'", class->dname);
                fc_error(fc);
            }
            if (!class->func_ref_weak) {
                sprintf(fc->sbuf, "Missing function '__ref_weak' in type '%s'", class->dname);
                fc_error(fc);
            }
            if (!class->func_deref_weak) {
                sprintf(fc->sbuf, "Missing function '__deref_weak' in type '%s'", class->dname);
                fc_error(fc);
            }
        }
        if (!class->func_free) {
            sprintf(fc->sbuf, "Missing function '__free' in type '%s'", class->dname);
            fc_error(fc);
        }
    }
}
