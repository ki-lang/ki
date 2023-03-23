
#include "../all.h"

void stage_2_class(Fc *fc, Class *class);
void stage_2_class_props(Fc *fc, Class *class, bool is_trait);
void stage_2_func(Fc *fc, Func *func);
void stage_2_class_defaults(Fc *fc, Class *class);
void stage_2_class_type_checks(Fc *fc, Class *class);
void stage_2_class_type_check(Fc *fc, Func *func, Type *args[], int argc, Type *rett, bool can_error);

void stage_2(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 2) {
        printf("# Stage 2 : Read types : %s\n", fc->path_ki);
    }

    b->core_types_scanned = true;

    for (int i = 0; i < fc->classes->length; i++) {
        Class *class = array_get_index(fc->classes, i);
        if (class->is_generic_base)
            continue;
        if (b->verbose > 2) {
            printf("> Scan class properties: %s\n", class->dname);
        }
        stage_2_class(fc, class);
    }
    for (int i = 0; i < fc->classes->length; i++) {
        Class *class = array_get_index(fc->classes, i);
        if (class->is_generic_base)
            continue;
        if (b->verbose > 2) {
            printf("> Class generate defaults: %s\n", class->dname);
        }
        stage_2_class_defaults(fc, class);
    }
    for (int i = 0; i < fc->funcs->length; i++) {
        Func *func = array_get_index(fc->funcs, i);
        if (!func->chunk_args)
            continue;
        if (b->verbose > 2) {
            printf("> Scan func types: %s\n", func->dname);
        }
        stage_2_func(fc, func);
    }
    for (int i = 0; i < fc->globals->length; i++) {
        Global *g = array_get_index(fc->globals, i);
        fc->chunk = g->type_chunk;
        Type *type = read_type(fc, fc->alc, fc->scope, true, true);
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
        if (b->verbose > 2) {
            printf("> Class type check internal functions: %s\n", class->dname);
        }
        stage_2_class_type_checks(fc, class);
    }

    //
    chain_add(b->stage_3, fc);
}

void stage_2_class(Fc *fc, Class *class) {

    //
    fc->chunk = class->chunk_body;
    stage_2_class_props(fc, class, false);

    // Generate __free / __deref / _RC
    if (class->type == ct_struct) {
        Build *b = fc->b;
        if (class->is_rc) {
            // Define _RC
            Type *type = type_gen(b, b->alc, "u32");
            ClassProp *prop = class_prop_init(b->alc, class, type);
            prop->value = vgen_vint(b->alc, 1, type, false);
            map_set(class->props, "_RC", prop);
        }
    }

    //
    if (class->type == ct_struct && class->props->keys->length == 0) {
        sprintf(fc->sbuf, "Class has no properties");
        fc_error(fc);
    }

    //
    if (!class_check_size(class)) {
        array_push(fc->class_size_checks, class);
    }
}

void stage_2_class_props(Fc *fc, Class *class, bool is_trait) {
    //
    char *token = fc->token;
    Scope *scope = class->scope;

    while (true) {

        tok(fc, token, false, true);
        // printf("t:'%s'\n", token);

        if (token[0] == 0) {
            sprintf(fc->sbuf, "Unexpected end of file");
            fc_error(fc);
        }

        if (strcmp(token, "}") == 0) {
            break;
        }

        if (strcmp(token, "/") == 0 && get_char(fc, 0) == '/') {
            skip_body(fc, '\n');
            continue;
        }

        if (strcmp(token, "#") == 0) {
            read_macro(fc, fc->alc, scope);
            continue;
        }

        if (strcmp(token, "trait") == 0) {

            if (is_trait) {
                sprintf(fc->sbuf, "You cannot use traits inside traits");
                fc_error(fc);
            }

            Id *id = read_id(fc, true, true, true);
            Idf *idf = idf_by_id(fc, scope, id, true);

            if (idf->type != idf_trait) {
                sprintf(fc->sbuf, "This is not a trait");
                fc_error(fc);
            }

            Trait *tr = idf->item;

            Chunk *current = fc->chunk;
            fc->chunk = chunk_clone(fc->alc, tr->chunk);

            stage_2_class_props(fc, class, true);

            fc->chunk = current;

            tok_expect(fc, ";", false, true);

            continue;
        }

        bool is_static = false;

        int act = act_public;

        if (strcmp(token, "public") == 0) {
            tok(fc, token, true, true);
        } else if (strcmp(token, "private") == 0) {
            act = act_private;
            tok(fc, token, true, true);
        } else if (strcmp(token, "readonly") == 0) {
            act = act_readonly;
            tok(fc, token, true, true);
        }

        if (strcmp(token, "static") == 0) {
            is_static = true;
            tok(fc, token, true, true);
        }

        if (strcmp(token, "func") == 0) {
            // Function
            tok(fc, token, true, true);

            bool take_ownership = false;
            if (strcmp(token, "+") == 0) {
                take_ownership = true;
                tok(fc, token, true, true);
            }

            if (!is_valid_varname(token)) {
                sprintf(fc->sbuf, "Invalid function name syntax: '%s'", token);
                fc_error(fc);
            }
            if (map_get(class->props, token) || map_get(class->funcs, token)) {
                sprintf(fc->sbuf, "Name already used (func name): '%s'", token);
                fc_error(fc);
            }

            Func *func = class_define_func(fc, class, is_static, token, NULL, NULL);
            func->act = act;

            if (!is_static && take_ownership) {
                Arg *arg = array_get_index(func->args, 0);
                arg->keep = false;
            }

            if (strcmp(func->name, "__ref") == 0) {
                class->func_ref = func;
                class->must_ref = true;
            } else if (strcmp(func->name, "__deref") == 0) {
                class->func_deref = func;
                class->must_deref = true;
            } else if (strcmp(func->name, "__free") == 0) {
                class->func_free = func;
            }

            tok_expect(fc, "(", true, true);

            func->chunk_args = chunk_clone(fc->alc, fc->chunk);

            skip_body(fc, ')');
            skip_until_char(fc, "{");
            func->chunk_body = chunk_clone(fc->alc, fc->chunk);
            skip_body(fc, '}');

        } else {
            // Property
            if (is_static) {
                sprintf(fc->sbuf, "Static properties are not allowed, use globals instead");
                fc_error(fc);
            }

            if (!is_valid_varname(token)) {
                sprintf(fc->sbuf, "Invalid function name syntax: '%s'", token);
                fc_error(fc);
            }
            if (map_get(class->props, token) || map_get(class->funcs, token)) {
                sprintf(fc->sbuf, "Name already used (property name): '%s'", token);
                fc_error(fc);
            }

            char *prop_name = dups(fc->alc, token);

            ClassProp *prop = class_prop_init(fc->alc, class, NULL);
            prop->act = act;

            tok(fc, token, true, true);
            if (strcmp(token, ":") == 0) {
                Type *type = read_type(fc, fc->alc, scope, true, true);
                if (type_is_void(type)) {
                    sprintf(fc->sbuf, "Cannot use 'void' as a type for a class property");
                    fc_error(fc);
                }
                prop->type = type;

                tok(fc, token, true, true);
            }

            if (strcmp(token, "=") == 0) {
                prop->value_chunk = chunk_clone(fc->alc, fc->chunk);
            } else {
                rtok(fc);
            }

            if (!prop->type && !prop->value_chunk) {
                sprintf(fc->sbuf, "The property must either have a type or default value defined. Both are missing.");
                fc_error(fc);
            }

            map_set(class->props, prop_name, prop);

            skip_until_char(fc, ";");
        }
    }
}

void stage_2_func(Fc *fc, Func *func) {
    //
    fc->chunk = func->chunk_args;
    char *token = fc->token;
    Allocator *alc = fc->alc;

    // Args
    tok(fc, token, true, true);
    while (strcmp(token, ")") != 0) {

        bool mutable = false;
        bool keep = true;

        if (strcmp(token, "mut") == 0) {
            mutable = true;
            tok(fc, token, true, true);
        }

        if (strcmp(token, "+") == 0) {
            keep = false;
            tok(fc, token, true, true);
        }

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

        Type *type = read_type(fc, alc, func->scope->parent, true, true);

        if (mutable && keep && type_tracks_ownership(type)) {
            sprintf(fc->sbuf, "If you use 'mut' you must also take ownership by typing '+' after it (for types that track ownership)");
            fc_error(fc);
        }

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

        Arg *arg = arg_init(alc, name, type, mutable);
        arg->keep = keep;
        arg->value_chunk = val_chunk;
        arg->type_chunk = type_chunk;

        array_push(func->args, arg);
        map_set(func->args_by_name, name, arg);
    }

    // Return type
    func->rett = read_type(fc, alc, func->scope->parent, true, true);

    Array *errors = NULL;

    tok(fc, token, false, true);

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
        if ((!type_is_void(rett) && class != b->class_i32) || rett->ptr_depth > 0) {
            sprintf(fc->sbuf, "func 'main' should return 'void' or 'i32'");
            fc_error(fc);
        }
    }

    // if (func == g_main_func) {
    //     //
    //     if (func->args->length > 1) {
    //         fc_error(fc, "main() too many arguments (max 1)", NULL);
    //     }
    //     if (func->args->length == 1) {
    //         // Replace args from main
    //         VarDecl *decl = array_get_index(func->args, 0);
    //         char *name = array_get_index(func->arg_names, 0);
    //         Type *type = decl->type;
    //         char hash[33];
    //         Array *types = array_make(1);
    //         array_push(types, type_gen_class(g_class_string));
    //         class_generate_generic_hash(types, hash);
    //         Class *class = class_get_generic_class(g_class_array, hash, types, true);
    //         if (type->class != class) {
    //             fc_error(fc, "main() argument must of type Array<String>", NULL);
    //         }
    //         g_main_func_arg_name = name;
    //         g_main_func_arg_mut = decl->is_mut;
    //         g_main_func_arg_type = decl->type;
    //         //
    //         VarDecl *argc = init_decl(false, true, type_gen_class(g_class_i32));
    //         VarDecl *argv = init_decl(false, true, type_gen_class(g_class_ptr));
    //         array_set_index(func->args, 0, argc);
    //         array_set_index(func->arg_names, 0, "KI_ARGC");
    //         array_set_index(func->arg_defaults, 0, NULL);
    //         array_push(func->args, argv);
    //         array_push(func->arg_names, "KI_ARGV");
    //         array_push(func->arg_defaults, NULL);
    //     }

    //     //
    //     if (func->return_type) {
    //         Class *class = func->return_type->class;
    //         if (!class || class != g_class_i32) {
    //             fc_error(fc, "main() must have a void or i32 return type", NULL);
    //         }
    //     }
    //     if (func->can_error) {
    //         fc_error(fc, "main() should not return errors", NULL);
    //     }
    // }
}

void stage_2_class_defaults(Fc *fc, Class *class) {

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
                if (pclass && pclass->must_deref) {
                    class->func_deref_props = class_define_func(fc, class, false, "__deref_props", NULL, type_gen_void(b->alc));
                    break;
                }
            }
        }
        // Define __free
        if (!class->func_free)
            class->func_free = class_define_func(fc, class, false, "__free", NULL, type_gen_void(b->alc));
    }

    //
    if (class->func_ref)
        class->func_ref->call_derefs = false;
    if (class->func_deref)
        class->func_deref->call_derefs = false;
    if (class->func_deref_props)
        class->func_deref_props->call_derefs = false;
    if (class->func_free)
        class->func_free->call_derefs = false;
}

void stage_2_class_type_checks(Fc *fc, Class *class) {
    //
    Func *func;
    Allocator *alc = fc->alc;

    Type *args[10];

    args[0] = type_gen_class(alc, class);

    func = map_get(class->funcs, "__ref");
    if (func)
        stage_2_class_type_check(fc, func, args, 1, type_gen_void(alc), false);
    func = map_get(class->funcs, "__deref");
    if (func)
        stage_2_class_type_check(fc, func, args, 1, type_gen_void(alc), false);
    func = map_get(class->funcs, "__free");
    if (func)
        stage_2_class_type_check(fc, func, args, 1, type_gen_void(alc), false);
    func = map_get(class->funcs, "__before_free");
    if (func)
        stage_2_class_type_check(fc, func, args, 1, type_gen_void(alc), false);

    // Iter
    Func *func_iter = map_get(class->funcs, "__iter_init");
    if (func_iter) {
        if (type_is_void(func_iter->rett)) {
            fc->chunk = func_iter->chunk_args;
            sprintf(fc->sbuf, "__iter_init can have any return type except 'void'");
            fc_error(fc);
        }
        stage_2_class_type_check(fc, func_iter, args, 1, NULL, false);
        Type *key_type = type_clone(alc, func_iter->rett);
        key_type->ptr_depth++;
        args[1] = func_iter->rett;
        args[2] = key_type;
        func = map_get(class->funcs, "__iter_get");
        if (func) {
            if (type_is_void(func->rett)) {
                fc->chunk = func->chunk_args;
                sprintf(fc->sbuf, "__iter_get can have any return type except 'void'");
                fc_error(fc);
            }
            stage_2_class_type_check(fc, func, args, 3, NULL, true);
            class->can_iter = true;
        }
    }
}
void stage_2_class_type_check(Fc *fc, Func *func, Type *args[], int argc, Type *rett, bool can_error) {
    //
    if (!func->chunk_args) {
        // Generated func
        return;
    }

    Chunk *chunk = fc->chunk;
    bool equal = func->args->length == argc;

    if (equal) {
        int i = 0;
        while (i < argc) {
            Type *type = args[i];
            Arg *arg = array_get_index(func->args, i);
            i++;
            if (!type_compat(type, arg->type, NULL)) {
                if (arg->type_chunk) {
                    chunk = arg->type_chunk;
                }
                equal = false;
                break;
            }
        }
    }
    if (equal && rett && !type_compat(rett, func->rett, NULL)) {
        equal = false;
    }
    if (equal && func->can_error != can_error) {
        equal = false;
    }

    if (!equal) {
        char buf[256];
        char expected[256];
        strcpy(expected, "fn(");
        int i = 0;
        while (i < argc) {
            if (i > 0) {
                strcat(expected, ", ");
            }
            Type *type = args[i];
            i++;
            type_to_str(type, buf);
            strcat(expected, buf);
        }
        strcat(expected, ")(");
        if (rett) {
            type_to_str(rett, buf);
            strcat(expected, buf);
        } else {
            strcat(expected, "{ANY}");
        }
        strcat(expected, ")");
        if (can_error) {
            strcat(expected, "!");
        }
        //
        type_to_str(type_gen_fptr(fc->alc, func), buf);
        //
        fc->chunk = chunk;
        sprintf(fc->sbuf, "Expected type for '%s' should be '%s', but was '%s'", func->dname, expected, buf);
        fc_error(fc);
    }
}
