
#include "../all.h"

void stage_2_2(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 2) {
        printf("# Stage 2.2 : Read class properties : %s\n", fc->path_ki);
    }

    for (int i = 0; i < fc->classes->length; i++) {
        Class *class = array_get_index(fc->classes, i);
        if (class->is_generic_base)
            continue;

        fc->chunk = class->chunk_body;
        stage_2_2_class_read_props(fc, class, false, false);
    }

    //
    chain_add(fc->b->stage_2_3, fc);
}

void stage_2_2_class_read_props(Fc *fc, Class *class, bool is_trait, bool is_extend) {
    //
    char *token = fc->token;
    Scope *scope = class->scope;

    Class *prev_error_class_info = fc->error_class_info;
    fc->error_class_info = class;

    if (fc->b->verbose > 2) {
        printf("> Read class properties: %s\n", class->dname);
    }

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
            Scope *trait_scope = scope_init(fc->alc, sct_default, tr->fc->scope, false);
            map_set(trait_scope->identifiers, "CLASS", map_get(scope->identifiers, "CLASS"));
            class->scope = trait_scope;

            stage_2_2_class_read_props(fc, class, true, is_extend);

            class->scope = scope;
            fc->chunk = current;

            tok_expect(fc, ";", false, true);

            continue;
        }

        bool is_static = false;

        int act = act_public;

        if (strcmp(token, "-") == 0) {
            act = act_private;
            tok(fc, token, true, true);
        } else if (strcmp(token, "~") == 0) {
            act = act_readonly;
            tok(fc, token, true, true);
        }

        Chunk *def_chunk = chunk_clone(fc->alc, fc->chunk);

        char next_token[KI_TOKEN_MAX];
        tok(fc, next_token, true, true);

        if (strcmp(token, "static") == 0 && strcmp(next_token, "fn") == 0) {
            is_static = true;
            strcpy(token, next_token);
        } else {
            rtok(fc);
        }

        if (strcmp(token, "stores_references_to") == 0) {

            Type *type = read_type(fc, fc->alc, scope, true, true, rtc_prop_type);
            tok_expect(fc, ";", false, true);

            array_push(class->refers_to_types, type);
            array_push(class->refers_to_names, "*");

        } else if (strcmp(token, "fn") == 0) {
            // Function
            *def_chunk = *fc->chunk;
            tok(fc, token, true, true);

            bool borrow = true;
            bool ref = false;
            bool will_exit = false;

            if (strcmp(token, "!") == 0) {
                will_exit = true;
                *def_chunk = *fc->chunk;
                tok(fc, token, true, false);
            }

            if (!is_static) {
                if (strcmp(token, ">") == 0) {
                    borrow = false;
                    *def_chunk = *fc->chunk;
                    tok(fc, token, true, false);
                }
            }

            if (!is_valid_varname(token)) {
                sprintf(fc->sbuf, "Invalid function name syntax: '%s'", token);
                fc_error(fc);
            }
            if (map_get(class->props, token) || map_get(class->funcs, token)) {
                sprintf(fc->sbuf, "Name already used (func name): '%s'", token);
                fc_error(fc);
            }

            Func *func = class_define_func(fc, class, is_static, token, NULL, NULL, fc->chunk->line);
            func->act = act;
            func->will_exit = will_exit;
            func->def_chunk = def_chunk;

            if (!is_static) {
                Arg *arg = array_get_index(func->args, 0);
                if (type_tracks_ownership(arg->type)) {
                    arg->type->borrow = borrow;
                    arg->type->shared_ref = ref;
                }
            }

            if (strcmp(func->name, "__ref") == 0) {
                class->func_ref = func;
            } else if (strcmp(func->name, "__deref") == 0) {
                class->func_deref = func;
            } else if (strcmp(func->name, "__ref_weak") == 0) {
                class->func_ref_weak = func;
            } else if (strcmp(func->name, "__deref_weak") == 0) {
                class->func_deref_weak = func;
            } else if (strcmp(func->name, "__free") == 0) {
                class->func_free = func;
            } else if (strcmp(func->name, "__before_free") == 0) {
                class->func_before_free = func;
            }

            tok(fc, token, true, true);
            if (strcmp(token, "(") == 0) {
                func->chunk_args = chunk_clone(fc->alc, fc->chunk);
                skip_body(fc, ')');
            } else {
                rtok(fc);
                func->chunk_args = chunk_clone(fc->alc, fc->chunk);
                func->parse_args = false;
            }

            skip_until_char(fc, "{");
            func->chunk_body = chunk_clone(fc->alc, fc->chunk);
            skip_body(fc, '}');

        } else {
            // Property
            if (is_extend) {
                sprintf(fc->sbuf, "You cannot define new properties using 'extend'");
                fc_error(fc);
            }
            if (is_static) {
                sprintf(fc->sbuf, "Static properties are not allowed, use globals instead");
                fc_error(fc);
            }

            if (!is_valid_varname(token)) {
                sprintf(fc->sbuf, "Invalid property name syntax: '%s'", token);
                fc_error(fc);
            }
            if (map_get(class->props, token) || map_get(class->funcs, token)) {
                sprintf(fc->sbuf, "Name already used (property name): '%s'", token);
                fc_error(fc);
            }

            char *prop_name = dups(fc->alc, token);

            ClassProp *prop = class_prop_init(fc->alc, class, NULL);
            prop->act = act;
            prop->def_chunk = def_chunk;

            tok_expect(fc, ":", true, true);

            // tok(fc, token, true, true);
            // if (strcmp(token, ":") == 0) {
            Type *type = read_type(fc, fc->alc, scope, true, true, rtc_prop_type);
            if (type_is_void(type)) {
                sprintf(fc->sbuf, "Cannot use 'void' as a type for a class property");
                fc_error(fc);
            }
            if (type->borrow) {
                class->has_borrows = true;
            }
            prop->type = type;

            tok(fc, token, true, true);
            // }

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
            array_push(class->refers_to_types, prop->type);
            array_push(class->refers_to_names, prop_name);

            skip_until_char(fc, ";");
        }
    }

    if (!is_trait && !is_extend) {

        // Define default properties
        if (class->type == ct_struct) {
            Build *b = fc->b;
            if (class->is_rc) {
                // Define _RC
                Type *type = type_gen(b, b->alc, "u32");
                ClassProp *prop = class_prop_init(b->alc, class, type);
                prop->value = vgen_vint(b->alc, 1, type, false);
                prop->act = act_private;
                map_set(class->props, "_RC", prop);

                Type *type_weak = type_gen(b, b->alc, "u32");
                ClassProp *prop_weak = class_prop_init(b->alc, class, type_weak);
                prop_weak->value = vgen_vint(b->alc, 0, type_weak, false);
                prop_weak->act = act_private;
                map_set(class->props, "_RC_WEAK", prop_weak);
            }

            if (class->props->keys->length == 0) {
                sprintf(fc->sbuf, "Class has no properties");
                fc_error(fc);
            }
        }

        // Check size
        if (!class_check_size(class)) {
            array_push(fc->class_size_checks, class);
        }
    }

    fc->error_class_info = prev_error_class_info;
}
