
#include "../all.h"

void fc_scan_types(FileCompiler *fc) {
    char *token = malloc(KI_TOKEN_MAX);
    bool has_read_namespace = false;
    bool changed_namespace = false;

    while (fc->i < fc->content_len) {
        fc_next_token(fc, token, false, false, true);
        // printf("t:%s\n", token);

        if (token[0] == '\0') {
            // printf("break\n");
            break;
        }

        if (strcmp(token, "namespace") == 0) {
            if (has_read_namespace == true) {
                fc_error(fc, "namespace token should be at the top of the file\n", NULL);
            }
            fc_next_token(fc, token, false, true, true);
            // Set namespace
            fc->nsc = pkc_get_namespace_or_create(fc->nsc->pkc, token);
            fc->scope->parent = fc->nsc->scope;
            //
            fc_expect_token(fc, ";", false, true, true);
            changed_namespace = true;
            continue;
        }
        if (!has_read_namespace) {
            if (!changed_namespace) {
                int index = array_find(cmd_arg_files, fc->ki_filepath, "chars");
                if (index == -1) {
                    fc_warn(fc,
                            "ki file loaded via namespace, but code doesnt define a "
                            "namespace (%s)",
                            fc->ki_filepath);
                }
            }
            has_read_namespace = true;
        }

        if (strcmp(token, "#") == 0) {
            fc_parse_macro(fc, fc->scope, token);

        } else if (strcmp(token, "enum") == 0) {
            fc_next_token(fc, token, false, true, true);
            fc_name_taken(fc, fc->nsc->scope->identifiers, token);

            Enum *enu = init_enum();
            enu->fc = fc;
            enu->name = strdup(token);

            array_push(fc->enums, enu);

            fc_read_enum_values(fc, enu);

            IdentifierFor *idf = init_idf();
            idf->type = idfor_enum;
            idf->item = enu;

            Scope *scope = fc->nsc->scope;
            map_set(scope->identifiers, enu->name, idf);

            char *cname = create_c_identifier_with_strings(fc->nsc->pkc->name, fc->nsc->name, enu->name);
            enu->cname = cname;
            map_set(c_identifiers, cname, idf);

        } else if (strcmp(token, "class") == 0) {
            fc_next_token(fc, token, false, true, true);
            fc_name_taken(fc, fc->nsc->scope->identifiers, token);

            char *name = strdup(token);

            Array *generic_names = NULL;
            fc_next_token(fc, token, true, true, true);
            if (strcmp(token, "<") == 0) {

                if (fc_get_char(fc, 0) != '<') {
                    fc_error(fc, "Remove the space between the class name and '<'", NULL);
                }
                // Read sub types
                generic_names = array_make(2);
                // Skip '<'
                fc_next_token(fc, token, false, true, true);
                fc_next_token(fc, token, false, true, true);
                while (strcmp(token, ">") != 0) {
                    if (!is_valid_varname(token)) {
                        fc_error(fc, "Invalid type placeholder name: '%s'", token);
                    }
                    array_push(generic_names, strdup(token));
                    fc_next_token(fc, token, true, true, true);
                    if (strcmp(token, ">") != 0) {
                        fc_expect_token(fc, ",", false, true, true);
                    }
                }
                // Skip '>'
                fc_next_token(fc, token, false, true, true);
            }

            Class *class = init_class();
            class->name = name;
            class->fc = fc;
            class->generic_names = generic_names;
            class->scope = init_sub_scope(fc->scope);
            class->scope->type = sct_class;
            class->scope->class = class;

            if (generic_names) {
                class->fc->was_modified = true;
                class->fc->should_recompile = true;
                class->fc->cache->depends_on = map_make();
            }

            array_push(fc->classes, class);
            array_push(g_classes, class);

            while (true) {
                fc_next_token(fc, token, true, true, true);

                if (strcmp(token, "norfc") == 0) {
                    fc_next_token(fc, token, false, true, true);
                    class->ref_count = false;
                    continue;
                }

                if (strcmp(token, "ctype") == 0) {
                    fc_next_token(fc, token, false, true, true);
                    // class->is_ctype = true;
                    continue;
                }

                if (strcmp(token, "number") == 0 || strcmp(token, "float") == 0) {
                    fc_next_token(fc, token, false, true, true);
                    class->ref_count = false;
                    class->is_number = true;
                    class->is_float = strcmp(token, "float") == 0;
                    fc_expect_token(fc, ":", false, true, false);
                    fc_next_token(fc, token, false, true, false);
                    if (strcmp(token, "u") == 0) {
                        class->is_unsigned = true;
                    } else if (strcmp(token, "s") == 0) {
                        class->is_unsigned = false;
                    } else {
                        fc_error(fc, "Expected 'u' or 'p', instead of: '%s'", token);
                    }
                    fc_expect_token(fc, ":", false, true, false);
                    fc_next_token(fc, token, false, true, false);
                    if (!is_valid_number(token)) {
                        fc_error(fc, "Invalid number: '%s'", token);
                    }
                    class->size = atoi(token);
                    if (class->size < 1) {
                        fc_error(fc, "Number byte size must be atleast 1, got: '%s'", token);
                    }
                    continue;
                }
                break;
            }

            IdentifierFor *idf = init_idf();
            idf->type = idfor_class;
            idf->item = class;

            Scope *scope = fc->nsc->scope;
            map_set(scope->identifiers, name, idf);
            map_set(class->scope->identifiers, "CLASS", idf);

            char *cname = create_c_identifier_with_strings(fc->nsc->pkc->name, fc->nsc->name, name);
            class->cname = cname;
            map_set(c_identifiers, cname, idf);

            fc_scan_class(fc, class);

        } else if (strcmp(token, "trait") == 0) {
            fc_next_token(fc, token, false, true, true);
            fc_name_taken(fc, fc->nsc->scope->identifiers, token);

            char *name = strdup(token);

            fc_expect_token(fc, "{", false, true, true);

            char *cname = create_c_identifier_with_strings(fc->nsc->pkc->name, fc->nsc->name, name);
            Trait *trait = init_trait();
            trait->fc = fc;
            trait->cname = cname;
            trait->body_i = fc->i;

            IdentifierFor *idf = init_idf();
            idf->type = idfor_trait;
            idf->item = trait;

            Scope *scope = fc->nsc->scope;
            map_set(scope->identifiers, name, idf);

            free(name);

            fc_skip_body(fc, "{", "}", NULL, false);

        } else if (strcmp(token, "func") == 0) {

            Function *func = init_func();
            func->fc = fc;
            func->scope = init_sub_scope(fc->scope);
            func->scope->is_func = true;
            func->scope->func = func;

            // Get name
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

            fc_name_taken(fc, fc->nsc->scope->identifiers, token);

            char *name = strdup(token);
            char *cname = create_c_identifier_with_strings(fc->nsc->pkc->name, fc->nsc->name, name);
            func->cname = cname;

            array_push(fc->functions, func);
            array_push(g_functions, func);

            if (strcmp(cname, "main") == 0) {
                g_main_func = func;
            }

            func->is_test = starts_with(name, "_test_");

            if (func->is_test) {
                array_push(g_test_funcs, func);
            }

            if (!func->is_test || g_run_tests) {
                //
                IdentifierFor *idf = init_idf();
                idf->type = idfor_func;
                idf->item = func;

                Scope *scope = fc->nsc->scope;
                map_set(scope->identifiers, name, idf);

                map_set(c_identifiers, cname, idf);

                fc_scan_func(fc, func);
            }

        } else if (strcmp(token, "converter") == 0) {
            fc_next_token(fc, token, true, true, true);
            if (!is_valid_varname(token)) {
                fc_error(fc, "Invalid name: '%s'", token);
            }

            Identifier *id = fc_read_identifier(fc, false, true, true);
            Scope *idf_scope = fc_get_identifier_scope(fc, fc->scope, id);
            IdentifierFor *idf = idf_find_in_scope(idf_scope, id);

            if (idf && idf->type != idfor_converter) {
                fc_error(fc, "Name already used for something else: '%s'", token);
            }

            char *cname = create_c_identifier_with_strings(fc->nsc->pkc->name, fc->nsc->name, token);

            fc_expect_token(fc, "{", false, false, true);

            Converter *cv = NULL;
            if (idf) {
                cv = idf->item;
                free(cname);
            } else {
                cv = malloc(sizeof(Converter));
                cv->cname = cname;
                cv->from_types = array_make(2);
                cv->to_types = array_make(2);
                cv->functions = array_make(2);

                IdentifierFor *idf = init_idf();
                idf->type = idfor_converter;
                idf->item = cv;

                map_set(c_identifiers, cname, idf);
            }

            ConverterPos *cvp = malloc(sizeof(ConverterPos));
            cvp->fc_i = fc->i;
            cvp->converter = cv;

            array_push(fc->converter_positions, cvp);

            fc_skip_body(fc, "{", "}", NULL, false);

        } else if (strcmp(token, "use") == 0) {
            fc_next_token(fc, token, false, true, true);
            if (!is_valid_varname(token)) {
                fc_error(fc, "Invalid name: '%s'", token);
            }
            char *package = NULL;
            char *namespace = strdup(token);
            if (fc_get_char(fc, 0) == ':') {
                fc->i++;
                fc_next_token(fc, token, false, true, false);
                if (!is_valid_varname(token)) {
                    fc_error(fc, "Invalid name: '%s'", token);
                }
                package = namespace;
                namespace = strdup(token);
            }

            PkgCompiler *pkc = fc->nsc->pkc;
            if (package != NULL) {
                pkc = pkc_get_by_name(package);
                free(package);
            }
            if (!pkc_namespace_exists(pkc, namespace)) {
                fc_error(fc, "Unknown namespace '%s'", namespace);
            }
            NsCompiler *nsc = pkc_get_namespace_or_create(pkc, namespace);

            IdentifierFor *idf = map_get(fc->scope->identifiers, namespace);
            if (idf) {
                fc_error(fc, "Identifier '%s' already used in this file", namespace);
            }

            idf = init_idf();
            idf->type = idfor_namespace;
            idf->item = nsc;

            map_set(fc->scope->identifiers, namespace, idf);

            fc_expect_token(fc, ";", false, true, true);

        } else if (strcmp(token, "unsafe_shared_global") == 0) {
            fc_define_global(fc, gv_shared, token);
        } else if (strcmp(token, "threaded_global") == 0) {
            fc_define_global(fc, gv_threaded, token);
        } else {
            fc_error(fc, "Unexpected token: '%s'", token);
        }
    }

    if (fc->macro_results->length > 0) {
        fc_error(fc, "Reached end of file, but some macro if statements were not closed", NULL);
    }

    free(token);
}

void fc_define_global(FileCompiler *fc, int type, char *token) {

    GlobalVar *gv = malloc(sizeof(GlobalVar));
    gv->fc = fc;
    gv->fc_i = fc->i;
    gv->type = type;

    fc_skip_type(fc);
    fc_next_token(fc, token, false, true, true);
    fc_name_taken(fc, fc->nsc->scope->identifiers, token);
    //
    gv->name = strdup(token);

    if (type == gv_shared) {
        if (!fc->is_header && !starts_with(gv->name, "GS_")) {
            fc_error(fc, "Shared global variable names must start with 'GS_'", NULL);
        }
    } else if (type == gv_threaded) {
        if (!fc->is_header && !starts_with(gv->name, "GT_")) {
            fc_error(fc, "Shared global variable names must start with 'GT_'", NULL);
        }
    }

    char *cname = create_c_identifier_with_strings(fc->nsc->pkc->name, fc->nsc->name, gv->name);
    gv->cname = cname;

    array_push(fc->globals, gv);

    fc_expect_token(fc, ";", false, true, true);
}
