
#include "../all.h"
#include "../libs/md5.h"

Class *init_class() {
    Class *class = malloc(sizeof(Class));
    class->name = NULL;
    class->cname = NULL;
    class->hash = NULL;
    class->fc = NULL;
    class->scope = NULL;
    class->ref_count = true;
    class->is_number = false;
    class->is_float = false;
    class->is_unsigned = false;
    class->is_ctype = false;
    class->self_scan = false;
    class->size = 0;
    class->llvm_type = NULL;
    class->props = map_make();
    class->struct_props = map_make();
    class->traits = array_make(2);
    class->generic_names = NULL;
    class->generic_hash = NULL;
    return class;
}

void free_class(Class *class) {
    //
    free(class->name);
    // todo: free props
    //
    free(class);
}

ClassProp *init_class_prop() {
    ClassProp *prop = malloc(sizeof(ClassProp));
    prop->access_type = acct_public;
    prop->struct_index = 0;
    prop->is_static = false;
    prop->is_func = false;
    prop->generate_code = true;
    prop->return_type = NULL;
    prop->default_value = NULL;
    prop->value_i = 0;
    prop->func = NULL;
    prop->macro_tag = NULL;
    return prop;
}

void free_class_prop(ClassProp *prop) {
    //
    free_type(prop->return_type);
    //
    free(prop);
}

Class *fc_make_generic_class(Class *class) {
    Class *gclass = malloc(sizeof(Class));
    *gclass = *class;
    gclass->props = map_make();
    gclass->traits = array_make(2);
    gclass->scope = init_sub_scope(class->fc->scope);
    gclass->scope->type = sct_class;
    gclass->scope->class = gclass;

    IdentifierFor *idf = init_idf();
    idf->type = idfor_class;
    idf->item = gclass;

    map_set(gclass->scope->identifiers, "CLASS", idf);

    if (build_ast_stage) {
        gclass->self_scan = true;
    }

    return gclass;
}

Class *fc_get_generic_class(FileCompiler *fc, Class *class, Scope *scope) {
    //
    fc_depends_on(class->fc, fc);
    //
    int fci = fc->i;
    char *uid = fc_class_read_generic_unique_id(fc, scope);
    char *cname = malloc(KI_TOKEN_MAX);
    strcpy(cname, class->cname);
    strcat(cname, "__");
    strcat(cname, uid);
    free(uid);

    Class *gclass = NULL;
    IdentifierFor *idf = map_get(c_identifiers, cname);
    if (idf) {
        gclass = idf->item;
    } else {
        Array *types = array_make(class->generic_names->length);
        // Read generic types
        fc->i = fci;
        fc_expect_token(fc, "<", false, true, true);
        int generic_c = 0;
        while (generic_c < class->generic_names->length) {
            Type *gen_t = fc_read_type(fc, scope);
            array_push(types, gen_t);
            generic_c++;
            if (generic_c < class->generic_names->length) {
                fc_expect_token(fc, ",", false, true, true);
            }
        }
        fc_expect_token(fc, ">", false, true, true);

        gclass = fc_get_generic_class_by_hash(class, types);

        // array_free(types); // shouldnt free values
    }

    free(cname);

    return gclass;
}

Class *fc_get_generic_class_by_hash(Class *class, Array *types) {
    //
    char *uid = types_to_generic_hash(types);
    //
    char *cname = malloc(KI_TOKEN_MAX);
    strcpy(cname, class->cname);
    strcat(cname, "__");
    strcat(cname, uid);

    Class *gclass = NULL;
    IdentifierFor *idf = map_get(c_identifiers, cname);
    if (idf) {
        gclass = idf->item;
        free(uid);
    } else {
        gclass = fc_make_generic_class(class);
        gclass->cname = strdup(cname);
        gclass->generic_hash = uid;
        // Types
        if (types->length != class->generic_names->length) {
            die("Not enough types to generate generic");
        }
        //
        for (int i = 0; i < types->length; i++) {
            Type *gen_t = array_get_index(types, i);
            char *name = array_get_index(class->generic_names, i);
            IdentifierFor *idf = init_idf();
            idf->type = idfor_type;
            idf->item = gen_t;
            map_set(gclass->scope->identifiers, name, idf);
        }

        // Add to class lists
        array_push(gclass->fc->classes, gclass);

        IdentifierFor *idf = init_idf();
        idf->type = idfor_class;
        idf->item = gclass;

        map_set(c_identifiers, gclass->cname, idf);

        char *vn = malloc(KI_TOKEN_MAX);
        strcpy(vn, gclass->name);
        strcat(vn, "__");
        strcat(vn, uid);

        map_set(gclass->fc->nsc->scope->identifiers, vn, idf);

        // Scan class
        if (gclass->self_scan) {
            fc_scan_class_props(gclass);
            fc_scan_class_prop_values(gclass);
            // map_print_keys(gclass->props);

            // for (int y = 0; y < gclass->props->values->length; y++) {
            //     char *n = array_get_index(gclass->props->keys, y);
            //     ClassProp *prop = array_get_index(gclass->props->values, y);
            //     if (prop->is_func) {

            //         Function *func = prop->func;

            //         if (!gclass->fc->is_header && prop->generate_code) {
            //             char *prev = gclass->fc->add_use_target;
            //             gclass->fc->add_use_target = func->cname;
            //             //
            //             fc_build_ast(func->fc, func->scope);
            //             //
            //             gclass->fc->add_use_target = prev;
            //         }

            //         Token *t = init_token();
            //         t->type = tkn_func;
            //         t->item = func;
            //         array_push(gclass->fc->scope->ast, t);
            //     }
            // }
        }
    }

    free(cname);

    return gclass;
}

char *types_to_generic_hash(Array *subtypes) {
    //
    Str *uid = str_make("|");
    for (int i = 0; i < subtypes->length; i++) {
        if (i > 0) {
            str_append_chars(uid, ",");
        }
        Type *subtype = array_get_index(subtypes, i);
        char *name = type_to_str(subtype);
        str_append_chars(uid, name);
        free(name);
    }
    str_append_chars(uid, "|");

    char *uidchars = str_to_chars(uid);
    free_str(uid);

    char *hash = malloc(33);
    strcpy(hash, "");
    md5(uidchars, hash);
    free(uidchars);

    return hash;
}

char *fc_class_read_generic_unique_id(FileCompiler *fc, Scope *scope) {
    //
    Str *uid = str_make("|");
    char *token = malloc(KI_TOKEN_MAX);

    fc_expect_token(fc, "<", false, true, true);

    fc_next_token(fc, token, true, true, true);
    while (strcmp(token, ">") != 0) {
        // Identifier* id = fc_read_identifier(fc, false, true, true);
        Type *gen_t = fc_read_type(fc, scope);
        char *name = type_to_str(gen_t);
        str_append_chars(uid, name);
        free(name);

        fc_next_token(fc, token, true, true, true);
        if (strcmp(token, ">") != 0) {
            fc_expect_token(fc, ",", false, true, true);
            str_append_chars(uid, ",");
        }
    }

    fc_expect_token(fc, ">", false, true, true);
    str_append_chars(uid, "|");

    //
    char *uidchars = str_to_chars(uid);
    free_str(uid);

    char *hash = malloc(33);
    strcpy(hash, "");
    md5(uidchars, hash);

    free(uidchars);

    return hash;
}

void fc_scan_class(FileCompiler *fc, Class *class) {
    //
    fc_expect_token(fc, "{", false, true, true);
    class->body_i = fc->i;
    // Look for class enums
    char *token = malloc(KI_TOKEN_MAX);

    while (fc->i < fc->content_len) {
        fc_next_token(fc, token, false, false, true);

        if (token[0] == '\0') {
            fc_error(fc, "Unexpected end of file, expected a '}' to close the class", NULL);
        }

        if (strcmp(token, "}") == 0) {
            break;
        }

        if (strcmp(token, "enum") == 0) {
            fc_next_token(fc, token, false, true, true);
            fc_name_taken(fc, class->props, token);

            Enum *enu = init_enum();
            enu->name = strdup(token);
            enu->fc = fc;

            ClassProp *prop = init_class_prop();
            prop->access_type = acct_public;
            prop->is_static = true;
            Type *type = init_type();
            type->type = type_enum;
            type->enu = enu;
            prop->return_type = type;

            map_set(class->props, enu->name, prop);

            fc_read_enum_values(fc, enu);

            continue;
        }

        if (strcmp(token, "trait") == 0) {
            Identifier *id = fc_read_identifier(fc, false, true, true);
        }

        if (strcmp(token, "\"") == 0) {
            fc_skip_string(fc);
            continue;
        }

        if (strcmp(token, "{") == 0) {
            fc_skip_body(fc, "{", "}", NULL, false);
            continue;
        }

        if (strcmp(token, "(") == 0) {
            fc_skip_body(fc, "(", ")", NULL, false);
            continue;
        }
    }
    class->body_i_end = fc->i;
}

void fc_scan_class_props(Class *class) {
    //
    if (class->generic_names != NULL && class->generic_hash == NULL) {
        return;
    }
    //
    char *token = malloc(KI_TOKEN_MAX);
    FileCompiler *fc = class->fc;
    fc->i = class->body_i;

    Array *chunks = array_make(2);

    //
    while (true) {
        fc_next_token(fc, token, false, false, true);

        if (token[0] == '\0' || strcmp(token, "}") == 0) {
            if (chunks->length > 0) {
                ContentChunk *cc = content_chunk_pop(chunks);
                fc = cc->fc;
                fc->i = cc->i;
                continue;
            }
            break;
        }

        // Macro
        if (strcmp(token, "#") == 0) {
            fc_parse_macro(fc, fc->scope, token);
            continue;
        }

        // Skip enum
        if (strcmp(token, "enum") == 0) {
            fc_skip_until_char(fc, ';');
            fc_expect_token(fc, ";", false, true, false);
            continue;
        }

        if (strcmp(token, "trait") == 0) {
            Identifier *id = fc_read_identifier(fc, false, true, true);
            Scope *idf_scope = fc_get_identifier_scope(fc, fc->scope, id);
            IdentifierFor *idf = idf_find_in_scope(idf_scope, id);
            if (!idf) {
                fc_error(fc, "Cannot find trait: %s", id->name);
            }
            if (idf->type != idfor_trait) {
                fc_error(fc, "Is not a trait: %s", id->name);
            }
            fc_expect_token(fc, ";", false, true, true);

            Trait *trait = idf->item;
            fc_depends_on(fc, trait->fc);

            ContentChunk *cc = content_chunk_create_for_fc(chunks, fc);
            // Scope* prev_scope = fc->scope;
            fc = trait->fc;
            fc->i = trait->body_i;
            // fc->scope = prev_scope;
            continue;
        }

        int acc_type = acct_unknown;
        if (strcmp(token, "public") == 0) {
            fc_next_token(fc, token, false, true, true);
            acc_type = acct_public;
        }
        if (strcmp(token, "private") == 0) {
            fc_next_token(fc, token, false, true, true);
            acc_type = acct_private;
        }
        if (strcmp(token, "readonly") == 0) {
            fc_next_token(fc, token, false, true, true);
            acc_type = acct_readonly;
        }
        if (acc_type == acct_unknown) {
            fc_error(fc, "Missing access type: public, private or readonly", NULL);
        }

        bool is_static = false;
        if (strcmp(token, "static") == 0) {
            fc_next_token(fc, token, false, true, true);
            is_static = true;
        }

        ClassProp *prop = init_class_prop();
        prop->access_type = acc_type;
        prop->is_static = is_static;

        if (fc->macro_tag) {
            prop->macro_tag = fc->macro_tag;
        }

        if (strcmp(token, "func") == 0) {

            Function *func = init_func();
            func->fc = fc;
            func->scope = init_sub_scope(class->scope);
            func->scope->is_func = true;
            func->scope->func = func;

            // Get func name
            fc_next_token(fc, token, false, true, true);

            if (strcmp(token, "|") == 0) {
                fc_next_token(fc, token, false, true, true);
                while (is_valid_varname(token)) {

                    if (strcmp(token, "used") == 0) {
                        fc->is_used = true;
                    }

                    fc_next_token(fc, token, false, true, true);
                    if (strcmp(token, ",") == 0) {
                        fc_next_token(fc, token, false, true, true);
                    }
                }
                fc_next_token(fc, token, false, true, true);
            }

            char *name = strdup(token);

            fc_name_taken(fc, class->props, name);

            Type *type = init_type();
            type->type = type_funcref;
            type->is_pointer = true;

            prop->return_type = type;
            prop->is_func = true;
            prop->func = func;

            map_set(class->props, name, prop);
            array_push(g_functions, func);

            char *cname = malloc(strlen(class->cname) + strlen(token) + 3);
            strcpy(cname, class->cname);
            strcat(cname, "__");
            strcat(cname, name);

            IdentifierFor *find = map_get(c_identifiers, cname);
            if (find != NULL) {
                fc_error(fc,
                         "The function its exportable name is the same as a previous "
                         "declaration: '%s'",
                         cname);
            }

            IdentifierFor *idf = init_idf();
            idf->type = idfor_func;
            idf->item = func;

            map_set(c_identifiers, cname, idf);
            func->cname = cname;

            if (!prop->is_static) {
                // first arg is "this"
                Type *t = init_type();
                t->type = type_struct;
                t->class = class;
                t->is_pointer = true;
                t->bytes = pointer_size;
                if (t->class->is_number) {
                    t->is_pointer = false;
                    t->allow_math = true;
                    t->bytes = class->size;
                }

                FunctionArg *arg = init_func_arg();
                arg->name = "this";
                arg->type = t;

                LocalVar *lv = malloc(sizeof(LocalVar));
                lv->name = "this";
                lv->gen_name = "this";
                lv->type = t;

                array_push(func->args, arg);
                array_push(func->arg_types, arg->type);
                IdentifierFor *thisidf = init_idf();
                thisidf->type = idfor_arg;
                thisidf->item = lv;
                map_set(func->scope->identifiers, "this", thisidf);
            }

            fc_expect_token(fc, "(", false, true, true);
            func->args_i = fc->i;
            func->args_i_end = fc->i + 5000;

            fc_scan_func_args(func);

            type->func_arg_types = func->arg_types;
            type->func_return_type = func->return_type;
            type->func_can_error = func->can_error;

            // fc_skip_until_char(fc, '{');
            // fc_expect_token(fc, "{", false, true, false);
            // func->scope->body_i = fc->i;
            fc_skip_body(fc, "{", "}", NULL, false);
            func->scope->body_i_end = fc->i;

            continue;
        }

        fc->i -= strlen(token);

        // Not a function, look for type
        Type *type = fc_read_type(fc, class->scope);
        prop->return_type = type;

        class->size += type->bytes;

        fc_next_token(fc, token, false, true, true);
        char *name = strdup(token);
        fc_name_taken(fc, class->props, name);
        map_set(class->props, name, prop);

        if (!prop->is_static) {
            prop->struct_index = class->struct_props->keys->length;
            map_set(class->struct_props, name, prop);
        }

        // printf("p:%s\n", name);
        // printf("c:%s\n", class->cname);
        // if (type->class) {
        //   printf("c:%s\n", type->class->cname);
        // }
        // printf("---\n");

        fc_next_token(fc, token, true, true, true);
        if (strcmp(token, "=") == 0) {
            fc_next_token(fc, token, false, true, true);
            prop->value_i = fc->i;
            fc_skip_body(fc, "(", ")", ";", true);
        } else {
            fc_expect_token(fc, ";", false, true, true);
        }
    }

    // Ref counting property
    if (class->ref_count) {
        // _RC
        ClassProp *prop = init_class_prop();
        prop->access_type = acct_public;
        prop->is_static = false;

        Type *type = fc_identifier_to_type(fc, create_identifier("ki", "type", "u16"), NULL);
        prop->return_type = type;

        Value *def_value = init_value();
        def_value->type = vt_number;
        def_value->item = "0";

        prop->default_value = def_value;

        class->size += type->bytes;
        map_set(class->props, "_RC", prop);

        prop->struct_index = class->struct_props->keys->length;
        map_set(class->struct_props, "_RC", prop);

        // Allocator
        // prop = init_class_prop();
        // prop->access_type = acct_public;
        // prop->is_static = false;

        // type = fc_identifier_to_type(fc, create_identifier("ki", "mem", "Allocator"), NULL);
        // prop->return_type = type;

        // class->size += type->bytes;
        // map_set(class->props, "_ALLOCATOR", prop);
    }

    ClassProp *bfp = map_get(class->props, "__before_free");
    if (bfp) {
        // Type check
        if (!bfp->is_func || bfp->is_static) {
            fc_error(fc, "__before_free must be a function (non-static)", NULL);
        }
        Function *func = bfp->func;
        if (func->args->length != 1) {
            fc_error(fc, "__before_free must have 0 arguments", NULL);
        }
        if (func->return_type != NULL) {
            fc_error(fc, "__before_free should not have a return type", NULL);
        }
        if (func->can_error) {
            fc_error(fc, "__before_free should not be allowed to throw an error", NULL);
        }
    }

    ClassProp *fp = map_get(class->props, "__free");
    if (!fp) {
        Function *func = init_func();
        func->fc = fc;
        func->scope = init_sub_scope(class->scope);
        func->scope->is_func = true;
        func->scope->func = func;

        Type *type = init_type();
        type->type = type_funcref;
        type->is_pointer = true;

        ClassProp *prop = init_class_prop();
        prop->access_type = acct_public;
        prop->is_static = false;
        prop->return_type = type;
        prop->is_func = true;
        prop->func = func;
        prop->generate_code = false;

        fp = prop;

        char *cname = malloc(strlen(class->cname) + 10);
        strcpy(cname, class->cname);
        strcat(cname, "____free");

        IdentifierFor *find = map_get(c_identifiers, cname);
        if (find != NULL) {
            fc_error(fc,
                     "The function its exportable name is the same as a previous "
                     "declaration: '%s'",
                     cname);
        }

        IdentifierFor *idf = init_idf();
        idf->type = idfor_func;
        idf->item = func;

        map_set(c_identifiers, cname, idf);
        func->cname = cname;

        Type *t = init_type();
        t->type = type_struct;
        t->class = class;
        t->is_pointer = true;
        t->bytes = pointer_size;
        if (t->class->is_number) {
            t->is_pointer = false;
            t->allow_math = true;
            t->bytes = class->size;
        }

        FunctionArg *arg = init_func_arg();
        arg->name = "this";
        arg->type = t;

        array_push(func->args, arg);
        array_push(func->arg_types, arg->type);

        type->func_arg_types = func->arg_types;
        type->func_return_type = func->return_type;
        type->func_can_error = func->can_error;

        map_set(class->props, "__free", prop);
    }
}

void fc_scan_class_prop_values(Class *class) {
    //
    if (class->generic_names != NULL && class->generic_hash == NULL) {
        return;
    }
    //

    FileCompiler *fc = class->fc;
    fc->add_use_target = class->cname;

    for (int o = 0; o < class->props->keys->length; o++) {
        //
        ClassProp *prop = array_get_index(class->props->values, o);
        //
        if (prop->value_i > 0) {
            fc->i = prop->value_i;
            prop->default_value = fc_read_value(fc, fc->scope, false, true, true);
            fc_expect_token(fc, ";", false, true, true);

            fc_type_compatible(fc, prop->return_type, prop->default_value->return_type);
        }
    }
}
