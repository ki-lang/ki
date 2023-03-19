
#include "../all.h"

void stage_2_class(Fc *fc, Class *class);
void stage_2_class_props(Fc *fc, Class *class, bool is_trait);
void stage_2_func(Fc *fc, Func *func);
void stage_2_class_defaults(Fc *fc, Class *class);

void stage_2(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 2) {
        printf("# Stage 2 : Read types : %s\n", fc->path_ki);
    }

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

        if (strcmp(token, "mut") == 0) {
            mutable = true;
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

        Chunk *val_chunk = NULL;

        tok_expect(fc, ":", true, true);
        Type *type = read_type(fc, alc, func->scope->parent, true, true);

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
        array_push(func->args, arg);
        map_set(func->args_by_name, name, arg);
    }

    // Define arguments in AST
    func_make_arg_decls(func);

    // Return type
    func->rett = read_type(fc, alc, func->scope->parent, true, true);

    // i = tok(fc, token, false, true);
    // if (strcmp(token, "or") == 0) {
    //     // Read error codes
    //     func->can_error = true;
    //     func->scope->can_error = true;
    //     fc->i = i;

    //     func->error_codes = array_make(2);
    //     while (true) {
    //         fc->i = tok(fc, token, true, true);
    //         if (!is_valid_varname(token)) {
    //             fc_error(fc, "Invalid error code syntax: '%s'", token);
    //         }
    //         if (array_contains(func->error_codes, token, arr_find_str)) {
    //             fc_error(fc, "Duplicate error code: '%s'", token);
    //         }

    //         array_push(func->error_codes, strdup(token));

    //         i = tok(fc, token, true, true);
    //         if (strcmp(token, ",") == 0) {
    //             fc->i = i;
    //             continue;
    //         }
    //         break;
    //     }
    // }

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