
#include "../all.h"

void stage_1_func(Fc *fc);
void stage_1_class(Fc *fc, bool is_struct);
void stage_1_trait(Fc *fc);
void stage_1_enum(Fc *fc);
void stage_1_use(Fc *fc);
void stage_1_header(Fc *fc);
void stage_1_link(Fc *fc, int link_type);
void stage_1_global(Fc *fc, bool shared);
void stage_1_alias(Fc *fc, int alias_type);
void stage_1_test(Fc *fc);
void stage_1_macro(Fc *fc);

void stage_1(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 2) {
        printf("# Stage 1 : Parse : %s\n", fc->path_ki);
    }

    char *token = fc->token;

    while (true) {

        tok(fc, token, false, true);

        if (token[0] == 0)
            break;

        if (strcmp(token, "#") == 0) {
            read_macro(fc, fc->alc, fc->scope);
            continue;
        }

        if (strcmp(token, "func") == 0) {
            stage_1_func(fc);
            continue;
        }
        if (strcmp(token, "class") == 0) {
            stage_1_class(fc, false);
            continue;
        }
        if (strcmp(token, "struct") == 0) {
            stage_1_class(fc, true);
            continue;
        }
        if (strcmp(token, "trait") == 0) {
            stage_1_trait(fc);
            continue;
        }
        if (strcmp(token, "enum") == 0) {
            stage_1_enum(fc);
            continue;
        }
        if (strcmp(token, "use") == 0) {
            stage_1_use(fc);
            continue;
        }
        if (strcmp(token, "header") == 0) {
            stage_1_header(fc);
            continue;
        }
        if (strcmp(token, "link") == 0) {
            stage_1_link(fc, link_default);
            continue;
        }
        if (strcmp(token, "link_dynamic") == 0) {
            stage_1_link(fc, link_dynamic);
            continue;
        }
        if (strcmp(token, "link_static") == 0) {
            stage_1_link(fc, link_static);
            continue;
        }
        if (strcmp(token, "global") == 0) {
            stage_1_global(fc, false);
            continue;
        }
        if (strcmp(token, "shared") == 0) {
            stage_1_global(fc, true);
            continue;
        }
        if (strcmp(token, "alias") == 0) {
            stage_1_alias(fc, alias_id);
            continue;
        }
        if (strcmp(token, "type_alias") == 0) {
            stage_1_alias(fc, alias_type);
            continue;
        }
        if (strcmp(token, "test") == 0) {
            stage_1_test(fc);
            continue;
        }
        if (strcmp(token, "macro") == 0) {
            stage_1_macro(fc);
            continue;
        }

        sprintf(fc->sbuf, "Unexpected token '%s'", token);
        fc_error(fc);
    }

    b->LOC += fc->chunk->line;

    //
    chain_add(b->stage_2, fc);
}

void stage_1_func(Fc *fc) {
    //
    Build *b = fc->b;
    char *token = fc->token;
    tok(fc, token, true, true);

    bool will_exit = false;
    if (strcmp(token, "!") == 0) {
        will_exit = true;
        tok(fc, token, true, false);
    }

    if (!is_valid_varname(token)) {
        sprintf(fc->sbuf, "Invalid function name syntax '%s'", token);
        fc_error(fc);
    }
    name_taken_check(fc, fc->nsc->scope, token);

    Func *func = func_init(fc->alc, fc->b);
    func->line = fc->chunk->line;

    bool is_main = fc->nsc == b->nsc_main && strcmp(token, "main") == 0;
    if (is_main) {
        strcpy(token, "ki__main__");
        if (b->lib_type == libt_exe) {
            b->main_func = func;
        }
    }

    char *name = dups(fc->alc, token);
    char *gname = nsc_gname(fc->nsc, name);
    char *dname = nsc_dname(fc->nsc, name);

    func->fc = fc;
    func->name = name;
    func->gname = gname;
    func->dname = dname;
    func->scope = scope_init(fc->alc, sct_func, fc->scope, true);
    func->scope->func = func;
    func->will_exit = will_exit;

    if (!is_main || b->lib_type == libt_exe)
        array_push(fc->funcs, func);

    Idf *idf = idf_init(fc->alc, idf_func);
    idf->item = func;

    if (is_main) {
        name = "main";
        func->dname = "main";
    }

    map_set(fc->nsc->scope->identifiers, name, idf);
    if (fc->is_header) {
        map_set(fc->scope->identifiers, name, idf);
    }

    tok_expect(fc, "(", true, true);

    func->chunk_args = chunk_clone(fc->alc, fc->chunk);

    skip_body(fc, ')');

    if (fc->is_header) {
        skip_until_char(fc, ";");
    } else {
        skip_until_char(fc, "{");
        func->chunk_body = chunk_clone(fc->alc, fc->chunk);
        skip_body(fc, '}');
    }
}

void stage_1_class(Fc *fc, bool is_struct) {
    //
    Build *b = fc->b;
    char *token = fc->token;
    tok(fc, token, true, true);

    if (!is_valid_varname(token)) {
        sprintf(fc->sbuf, "Invalid class name '%s'", token);
        fc_error(fc);
    }
    name_taken_check(fc, fc->nsc->scope, token);

    char *name = dups(fc->alc, token);
    char *gname = nsc_gname(fc->nsc, name);
    char *dname = nsc_dname(fc->nsc, name);

    Class *class = class_init(fc->alc);
    class->fc = fc;
    class->name = name;
    class->gname = gname;
    class->dname = dname;
    class->scope = scope_init(fc->alc, sct_class, fc->scope, false);
    class->is_struct = is_struct;
    class->is_rc = !is_struct;
    class->must_ref = !is_struct;
    class->must_deref = !is_struct;

    array_push(fc->classes, class);

    Idf *idf = idf_init(fc->alc, idf_class);
    idf->item = class;

    map_set(fc->nsc->scope->identifiers, name, idf);
    if (fc->is_header) {
        map_set(fc->scope->identifiers, name, idf);
    }

    map_set(class->scope->identifiers, "CLASS", idf);

    if (!b->core_types_scanned) {
        if (fc->nsc == b->nsc_type) {
            if (strcmp(name, "u8") == 0) {
                b->class_u8 = class;
            } else if (strcmp(name, "u16") == 0) {
                b->class_u16 = class;
            } else if (strcmp(name, "u32") == 0) {
                b->class_u32 = class;
            } else if (strcmp(name, "u64") == 0) {
                b->class_u64 = class;
            } else if (strcmp(name, "i8") == 0) {
                b->class_i8 = class;
            } else if (strcmp(name, "i16") == 0) {
                b->class_i16 = class;
            } else if (strcmp(name, "i32") == 0) {
                b->class_i32 = class;
            } else if (strcmp(name, "i64") == 0) {
                b->class_i64 = class;
            } else if (strcmp(name, "ptr") == 0) {
                b->class_ptr = class;
            } else if (strcmp(name, "String") == 0) {
                b->class_string = class;
            } else if (strcmp(name, "Array") == 0) {
                b->class_array = class;
            } else if (strcmp(name, "Map") == 0) {
                b->class_map = class;
            }
        }
    }

    if (get_char(fc, 0) == '[') {
        chunk_move(fc->chunk, 1);
        // Generic
        class->is_generic_base = true;
        class->generic_names = array_make(fc->alc, 4);
        class->generics = map_make(fc->alc);

        tok(fc, token, true, true);
        while (true) {
            if (!is_valid_varname(token)) {
                sprintf(fc->sbuf, "Invalid name");
                fc_error(fc);
            }
            name_taken_check(fc, class->scope, token);

            if (array_contains(class->generic_names, token, arr_find_str)) {
                sprintf(fc->sbuf, "Duplicate name");
                fc_error(fc);
            }

            char *name = dups(fc->alc, token);
            array_push(class->generic_names, name);

            tok(fc, token, true, true);
            if (strcmp(token, ",") == 0)
                tok(fc, token, true, true);
            if (strcmp(token, "]") == 0)
                break;
        }
    }

    tok(fc, token, true, true);
    while (strcmp(token, "{") != 0) {
        if (strcmp(token, "type") == 0) {
            tok_expect(fc, ":", true, false);
            tok(fc, token, true, false);
            if (strcmp(token, "ptr") == 0) {
                class->type = ct_ptr;
                class->is_rc = false;
                class->must_ref = false;
                class->must_deref = false;
                class->size = fc->b->ptr_size;
            } else if (strcmp(token, "int") == 0 || strcmp(token, "float") == 0) {
                class->type = strcmp(token, "int") == 0 ? ct_int : ct_float;
                class->is_rc = false;
                class->must_ref = false;
                class->must_deref = false;
                tok_expect(fc, ":", true, false);
                tok(fc, token, true, false);
                int size = 0;
                if (strcmp(token, "*") == 0) {
                    size = fc->b->ptr_size;
                } else {
                    if (!is_valid_number(token)) {
                        sprintf(fc->sbuf, "Invalid number byte size: '%s'", token);
                        fc_error(fc);
                    }
                    size = atoi(token);
                }
                if (class->type == ct_int) {
                    if (size != 1 && size != 2 && size != 4 && size != 8 && size != 16) {
                        sprintf(fc->sbuf, "Invalid integer byte size, options: 1,2,4,8 or 16 received: '%s'", token);
                        fc_error(fc);
                    }
                    tok_expect(fc, ":", true, false);
                    tok(fc, token, true, false);
                    if (strcmp(token, "true") != 0 && strcmp(token, "false") != 0) {
                        sprintf(fc->sbuf, "Invalid value for is_signed, options: true,false, received: '%s'", token);
                        fc_error(fc);
                    }
                    class->size = size;
                    class->is_signed = strcmp(token, "true") == 0;
                } else {
                    if (size != 4 && size != 8) {
                        sprintf(fc->sbuf, "Invalid float byte size, options: 4 or 8 received: '%s'", token);
                        fc_error(fc);
                    }
                    class->size = size;
                    class->is_signed = true;
                }
            } else {
                sprintf(fc->sbuf, "Unknown class type: '%s'", token);
                fc_error(fc);
            }
        } else if (class->is_rc && strcmp(token, "norc") == 0) {
            class->is_rc = false;
            class->must_ref = false;
            class->must_deref = false;
        } else if (strcmp(token, "packed") == 0) {
            class->packed = true;
        } else if (strcmp(token, "math") == 0) {
            class->allow_math = true;
        } else if (strcmp(token, "track") == 0) {
            class->track_ownership = true;
        } else if (strcmp(token, "async") == 0) {
            class->async = true;
        } else {
            sprintf(fc->sbuf, "Unexpected token: '%s' (class attributes)", token);
            fc_error(fc);
        }

        tok(fc, token, true, true);
    }

    class->chunk_body = chunk_clone(fc->alc, fc->chunk);

    skip_body(fc, '}');
}

void stage_1_trait(Fc *fc) {
    //
    char *token = fc->token;
    tok(fc, token, true, true);

    if (fc->is_header) {
        sprintf(fc->sbuf, "You cannot use 'trait' inside a header file");
        fc_error(fc);
    }
    if (!is_valid_varname(token)) {
        sprintf(fc->sbuf, "Invalid trait name syntax '%s'", token);
        fc_error(fc);
    }
    name_taken_check(fc, fc->nsc->scope, token);

    char *name = dups(fc->alc, token);
    char *gname = nsc_gname(fc->nsc, name);
    char *dname = nsc_dname(fc->nsc, name);

    Trait *tr = al(fc->alc, sizeof(Trait));
    tr->fc = fc;
    tr->name = name;
    tr->gname = gname;
    tr->dname = dname;

    tok_expect(fc, "{", false, true);

    Idf *idf = idf_init(fc->alc, idf_trait);
    idf->item = tr;

    map_set(fc->nsc->scope->identifiers, name, idf);

    tr->chunk = chunk_clone(fc->alc, fc->chunk);

    skip_body(fc, '}');
}

void stage_1_enum(Fc *fc) {
    //
    char *token = fc->token;
    tok(fc, token, true, true);

    if (!is_valid_varname(token)) {
        sprintf(fc->sbuf, "Invalid enum name '%s'", token);
        fc_error(fc);
    }
    name_taken_check(fc, fc->nsc->scope, token);

    char *name = dups(fc->alc, token);
    char *gname = nsc_gname(fc->nsc, name);
    char *dname = nsc_dname(fc->nsc, name);

    Enum *enu = al(fc->alc, sizeof(Enum));
    enu->fc = fc;
    enu->name = name;
    enu->gname = gname;
    enu->dname = dname;

    int autov = 0;

    tok_expect(fc, "{", false, true);

    Map *values = map_make(fc->alc);

    tok(fc, token, false, true);
    while (strcmp(token, "}") != 0) {
        if (!is_valid_varname(token)) {
            sprintf(fc->sbuf, "Invalid enum property name '%s'", token);
            fc_error(fc);
        }
        char *name = dups(fc->alc, token);
        tok(fc, token, false, true);
        if (strcmp(token, ":") == 0) {
            tok(fc, token, false, true);
            bool negative = false;
            if (strcmp(token, "-") == 0) {
                negative = true;
                tok(fc, token, true, false);
            }
            if (is_valid_number(token)) {
                int value = 0;
                if (strcmp(token, "0") == 0 && get_char(fc, 0) == 'x') {
                    // Hex
                    chunk_move(fc->chunk, 1);
                    read_hex(fc, token);
                    value = hex2int(token);
                } else {
                    // Number
                    value = atoi(token);
                }
                if (negative)
                    value = value * -1;

                map_set(values, name, (void *)(intptr_t)value);

                tok(fc, token, false, true);
                if (strcmp(token, ",") == 0) {
                    tok(fc, token, false, true);
                    continue;
                } else if (strcmp(token, "}") == 0) {
                    break;
                } else {
                    sprintf(fc->sbuf, "Expected ',' or '}' here");
                    fc_error(fc);
                }

            } else {
                sprintf(fc->sbuf, "Invalid enum property value '%s'", token);
                fc_error(fc);
            }
            continue;
        } else if (strcmp(token, ",") == 0) {
            int val = autov++;
            while (array_contains(values->values, (void *)(intptr_t)val, arr_find_adr)) {
                val = autov++;
            }
            map_set(values, name, (void *)(intptr_t)val);
            tok(fc, token, false, true);
            continue;
        } else if (strcmp(token, "}") == 0) {
            break;
        }

        sprintf(fc->sbuf, "Expected ':', ',' or '}' here");
        fc_error(fc);
    }

    enu->values = values;

    Idf *idf = idf_init(fc->alc, idf_enum);
    idf->item = enu;

    map_set(fc->nsc->scope->identifiers, name, idf);
    if (fc->is_header) {
        map_set(fc->scope->identifiers, name, idf);
    }
}

void stage_1_header(Fc *fc) {
    //
    tok_expect(fc, "\"", true, true);

    Str *buf = read_string(fc);
    Str *fnstr = macro_replace_str_vars(fc->alc, fc, buf);
    char *fn = str_to_chars(fc->alc, fnstr);
    Nsc *nsc_main = fc->b->nsc_main;

    Array *dirs = fc->pkc_config->header_dirs;
    bool found = false;
    for (int i = 0; i < dirs->length; i++) {
        char *dir = array_get_index(dirs, i);
        sprintf(fc->sbuf, "%s/%s.kh", dir, fn);

        if (file_exists(fc->sbuf)) {
            char *path = dups(fc->alc, fc->sbuf);
            Fc *hfc = fc_init(fc->b, path, nsc_main, fc->nsc->pkc, false);

            //
            if (fc->is_header) {
                array_push(fc->sub_headers, hfc);
            } else {
                tok_expect(fc, "as", true, true);

                char *token = fc->token;
                tok(fc, token, true, true);

                if (!is_valid_varname_char(token[0])) {
                    sprintf(fc->sbuf, "Invalid variable name syntax: '%s'", token);
                    fc_error(fc);
                }
                name_taken_check(fc, fc->scope, token);

                char *as = dups(fc->alc, token);

                //
                Idf *idf = idf_init(fc->alc, idf_fc);
                idf->item = hfc;

                map_set(fc->scope->identifiers, as, idf);
            }

            tok_expect(fc, ";", true, true);

            found = true;
            break;
        }
    }
    if (!found) {
        printf("Header directories: %d\n", dirs->length);
        for (int i = 0; i < dirs->length; i++) {
            char *dir = array_get_index(dirs, i);
            sprintf(fc->sbuf, "%s/%s.kh", dir, fn);
            printf("Lookup: %s\n", fc->sbuf);
        }

        sprintf(fc->sbuf, "Header not found: '%s.kh'\n", fn);
        fc_error(fc);
    }
}

void stage_1_link(Fc *fc, int link_type) {
    //
    tok_expect(fc, "\"", true, true);

    Str *buf = read_string(fc);
    Str *fnstr = macro_replace_str_vars(fc->alc, fc, buf);
    char *fn = str_to_chars(fc->alc, fnstr);

    if (link_type == link_default) {
        link_type = fc->b->link_static ? link_static : link_dynamic;
    }

    Link *link = map_get(fc->b->link_libs, fn);
    if (!link) {
        link = malloc(sizeof(Link));
        link->type = link_type;
        map_set(fc->b->link_libs, fn, link);
    }
    if (link_type == link_dynamic && link->type != link_dynamic) {
        link->type = link_dynamic;
    }

    tok_expect(fc, ";", true, true);
}

void stage_1_use(Fc *fc) {
    //
    Id *id = read_id(fc, true, true, true);

    char *pkc_name = id->has_nsc ? id->nsc_name : NULL;
    char *nsc_name = id->name;

    Pkc *pkc = fc->nsc->pkc;
    if (pkc_name) {
        pkc = pkc_get_sub_package(pkc, pkc_name);
        if (!pkc) {
            sprintf(fc->sbuf, "Unknown package: '%s'", pkc_name);
            fc_error(fc);
        }
    }

    Nsc *nsc = pkc_load_nsc(pkc, nsc_name, fc);

    char *as = nsc_name;
    char *token = fc->token;
    tok(fc, token, true, true);
    if (strcmp(token, "as") == 0) {
        tok(fc, token, true, true);
        if (!is_valid_varname(token)) {
            sprintf(fc->sbuf, "Invalid variable name syntax '%s'", token);
            fc_error(fc);
        }
        as = token;
    } else {
        rtok(fc);
    }
    name_taken_check(fc, fc->scope, as);

    Idf *idf = idf_init(fc->alc, idf_nsc);
    idf->item = nsc;

    map_set(fc->scope->identifiers, as, idf);

    tok_expect(fc, ";", true, true);
}

void stage_1_global(Fc *fc, bool shared) {
    //
    char *token = fc->token;
    tok(fc, token, true, true);

    if (!is_valid_varname(token)) {
        sprintf(fc->sbuf, "Invalid global name syntax '%s'", token);
        fc_error(fc);
    }
    name_taken_check(fc, fc->nsc->scope, token);

    char *name = dups(fc->alc, token);
    char *gname = nsc_gname(fc->nsc, name);
    char *dname = nsc_dname(fc->nsc, name);

    Global *g = al(fc->alc, sizeof(Global));
    g->fc = fc;
    g->name = name;
    g->gname = gname;
    g->dname = dname;
    g->shared = shared;
    g->type = NULL;

    array_push(fc->globals, g);

    tok_expect(fc, ":", true, true);

    g->type_chunk = chunk_clone(fc->alc, fc->chunk);

    skip_type(fc);

    tok_expect(fc, ";", true, true);

    Idf *idf = idf_init(fc->alc, idf_global);
    idf->item = g;

    map_set(fc->nsc->scope->identifiers, name, idf);
    if (fc->is_header) {
        map_set(fc->scope->identifiers, name, idf);
    }
}

void stage_1_alias(Fc *fc, int alias_type) {
    //
    Alias *a = malloc(sizeof(Alias));
    a->chunk = chunk_clone(fc->alc, fc->chunk);
    a->type = alias_type;

    skip_type(fc);

    tok_expect(fc, "as", true, true);

    char *token = fc->token;
    tok(fc, token, true, true);

    if (!is_valid_varname(token)) {
        sprintf(fc->sbuf, "Invalid global name syntax '%s'", token);
        fc_error(fc);
    }
    name_taken_check(fc, fc->nsc->scope, token);

    a->name = dups(fc->alc, token);

    tok_expect(fc, ";", true, true);

    array_push(fc->aliasses, a);
}

void stage_1_test(Fc *fc) {
    //
    int line = fc->chunk->line;
    char *token = fc->token;
    Allocator *alc = fc->alc;
    Build *b = fc->b;

    tok_expect(fc, "\"", true, true);

    Chunk *chu = fc->chunk;
    char *content = chu->content;
    int i = chu->i;
    Str *str = str_make(alc, 64);

    bool found = false;
    char ch = content[i];
    while (ch != '\0') {
        ch = content[i];
        i++;
        if (ch == '\n')
            break;
        if (ch == '\\') {
            str_append_char(str, '\\');
            str_append_char(str, ch);
            i++;
            continue;
        }
        if (ch == '"') {
            found = true;
            break;
        }
        str_append_char(str, ch);
    }
    if (!found) {
        sprintf(fc->sbuf, "Missing end of string");
        fc_error(fc);
    }
    chu->i = i;

    if (str->length > 128) {
        sprintf(fc->sbuf, "Test name too long");
        fc_error(fc);
    }

    char *body = str_to_chars(alc, str);

    tok_expect(fc, "{", false, true);

    // If tests enabled && is main package
    if (b->test && fc->nsc->pkc == b->nsc_main->pkc) {

        Func *func = func_init(fc->alc, fc->b);
        func->line = line;

        sprintf(token, "ki__TEST_%d__%s", ++fc->test_counter, fc->path_hash);

        char *name = dups(fc->alc, token);
        char *gname = nsc_gname(fc->nsc, name);
        char *dname = nsc_dname(fc->nsc, name);

        func->fc = fc;
        func->name = name;
        func->gname = gname;
        func->dname = dname;
        func->scope = scope_init(fc->alc, sct_func, fc->scope, true);
        func->scope->func = func;
        func->is_test = true;

        Test *test = al(alc, sizeof(Test));
        test->name = body;
        test->func = func;
        func->test = test;
        test->expects = NULL;

        Chunk *chunk = chunk_init(alc, fc);
        chunk->fc = fc;
        chunk->content = "ki__test__expect_count: u32[1], ki__test__success_count: u32[1], ki__test__fail_count: u32[1]) void {";
        chunk->length = strlen(chunk->content);

        func->chunk_args = chunk;
        func->chunk_body = chunk_clone(fc->alc, fc->chunk);

        array_push(fc->funcs, func);
        array_push(b->tests, test);
    }

    skip_body(fc, '}');
}

void stage_1_macro(Fc *fc) {
    //
    Build *b = fc->b;
    Allocator *alc = fc->alc;
    if (!b->macro_header_path) {
        sprintf(fc->sbuf, "Macros must be defined in 'macro.ki' located in the root of your project.");
        fc_error(fc);
    }
    if (!fc->macros)
        fc->macros = array_make(alc, 20);

    char *token = fc->token;
    tok(fc, token, true, true);

    if (!is_valid_varname(token)) {
        sprintf(fc->sbuf, "Invalid macro name syntax '%s'", token);
        fc_error(fc);
    }
    name_taken_check(fc, fc->nsc->scope, token);

    char *name = dups(alc, token);
    int len = strlen(name);
    char *dname = al(alc, len + 12);
    char *gname = al(alc, len + 12);
    sprintf(dname, "MACRO:%s", name);
    sprintf(gname, "KI__MACRO__%s", name);

    ///////////

    Macro *mac = al(alc, sizeof(Macro));

    Func *func = func_init(alc, fc->b);
    func->line = fc->chunk->line;
    func->fc = fc;
    func->name = name;
    func->gname = gname;
    func->dname = dname;
    func->scope = scope_init(alc, sct_func, fc->scope, true);
    func->scope->func = func;

    mac->func = func;

    array_push(fc->funcs, func);
    array_push(fc->macros, mac);

    Idf *idf = idf_init(alc, idf_func);
    idf->item = func;

    map_set(fc->nsc->scope->identifiers, name, idf);
    if (fc->is_header) {
        map_set(fc->scope->identifiers, name, idf);
    }

    ///////////

    tok_expect(fc, "\"", true, true);
    Str *buf = read_string(fc);
    char *pattern = str_to_chars(alc, buf);
    if (strcmp(pattern, "[]") == 0) {
    } else if (strcmp(pattern, "{}") == 0) {
    } else if (strcmp(pattern, "()") == 0) {
    } else {
        sprintf(fc->sbuf, "Invalid macro pattern: '%s'", pattern);
        fc_error(fc);
    }

    tok(fc, token, true, true);
    if (!is_valid_varname(token)) {
        sprintf(fc->sbuf, "Invalid variable name: '%s'", token);
        fc_error(fc);
    }
    char *var_code = dups(alc, token);

    tok_expect(fc, ",", true, true);

    tok(fc, token, true, true);
    if (!is_valid_varname(token)) {
        sprintf(fc->sbuf, "Invalid variable name: '%s'", token);
        fc_error(fc);
    }
    char *var_result = dups(alc, token);

    if (strcmp(var_code, var_result) == 0) {
        sprintf(fc->sbuf, "Variable names must be different: '%s' == '%s'", var_code, var_result);
        fc_error(fc);
    }

    mac->var_code = var_code;
    mac->var_result = var_result;
    mac->pattern = pattern;

    ///////////

    tok_expect(fc, "{", false, true);
    func->chunk_body = chunk_clone(fc->alc, fc->chunk);
    skip_body(fc, '}');
}
