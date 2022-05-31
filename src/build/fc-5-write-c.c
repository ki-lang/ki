
#include "../all.h"

void fc_write_c_all() {
    // Clear header
    char *path = malloc(KI_PATH_MAX);
    char *cache_dir = get_cache_dir();
    strcpy(path, cache_dir);
    strcat(path, "/project.h");
    write_file(path, "", false);

    for (int i = 0; i < headers->length; i++) {
        // char* fn = array_get_index(headers->keys, i);
        // bool is_syslib = strcmp(array_get_index(headers->keys, i), "1") == 0;
        // if (is_syslib) {
        //   write_file(path, "#include <", true);
        //   write_file(path, fn, true);
        //   write_file(path, ">\n", true);
        // } else {
        //   write_file(path, "#include \"", true);
        //   write_file(path, fn, true);
        //   write_file(path, "\"\n", true);
        // }
    }

    // Predefine all struct names
    for (int i = 0; i < packages->keys->length; i++) {
        PkgCompiler *pkc = array_get_index(packages->values, i);
        for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
            FileCompiler *fc = array_get_index(pkc->file_compilers->values, o);

            for (int x = 0; x < fc->classes->length; x++) {
                Class *class = array_get_index(fc->classes, x);
                if (class->generic_names != NULL && class->generic_hash == NULL) {
                    continue;
                }
                fc_write_c_predefine_class(fc, class);
            }

            fc_write_c_pre(fc);
        }
    }

    //
    for (int i = 0; i < packages->keys->length; i++) {
        PkgCompiler *pkc = array_get_index(packages->values, i);
        for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
            FileCompiler *fc = array_get_index(pkc->file_compilers->values, o);

            for (int x = 0; x < fc->classes->length; x++) {
                Class *class = array_get_index(fc->classes, x);
                if (class->generic_names != NULL && class->generic_hash == NULL) {
                    continue;
                }
                fc_write_c_class(fc, class);
            }
            for (int x = 0; x < fc->enums->length; x++) {
                fc_write_c_enum(fc, array_get_index(fc->enums, x));
            }
            for (int x = 0; x < fc->globals->length; x++) {
                fc_write_c_global(fc, array_get_index(fc->globals, x));
            }
            for (int x = 0; x < fc->strings->length; x++) {
                ValueString *vstr = array_get_index(fc->strings, x);
                str_append_chars(fc->h_code, "struct ki__type__String* ");
                str_append_chars(fc->h_code, vstr->name);
                str_append_chars(fc->h_code, ";\n");
            }

            fc_write_c_ast(fc, fc->scope);
            fc_write_c(fc);
        }
    }

    fc_write_c_inits();

    if (uses_async) {
        write_file(path, "void ki__async__Taskman__add_task(struct ki__async__Task* task);\n", true);
        write_file(path, "void ki__async__Taskman__run_another_task();\n", true);
    }
    write_file(path, "void KI_INITS();\n", true);
    write_file(path, "void* KI_ALLOCATORS;\n", true);
    write_file(path, "void* KI_ALLOCATORS_MUT;\n", true);
}

void fc_write_c_pre(FileCompiler *fc) {
    char *structs_code = str_to_chars(fc->struct_code);

    char *path = malloc(KI_PATH_MAX);
    char *cache_dir = get_cache_dir();
    strcpy(path, cache_dir);
    strcat(path, "/project.h");
    write_file(path, structs_code, true);

    free(structs_code);
}

void fc_write_c_inits() {
    Str *code = str_make("");
    //
    char *hpath = malloc(KI_PATH_MAX);
    char *cache_dir = get_cache_dir();
    strcpy(hpath, cache_dir);
    strcat(hpath, "/project.h");
    //
    str_append_chars(code, "#include \"project.h\"\n\n");

    FileCompiler *inits_fc = init_fc();

    str_append(code, inits_fc->c_code_after);
    write_file(hpath, str_to_chars(inits_fc->h_code), true);

    str_append_chars(code, "void KI_INITS(){\n");

    for (int i = 0; i < allocators->keys->length; i++) {
        char *name = array_get_index(allocators->values, i);
        if (name[strlen(name) - 1] == '0') {
            str_append_chars(code, name);
            str_append_chars(code, "__TK = (void*) 0;\n");
        } else {
            str_append_chars(code, "pthread_key_create(&");
            str_append_chars(code, name);
            str_append_chars(code, "__TK, (void*)0);\n");
        }
    }

    for (int i = 0; i < packages->keys->length; i++) {
        PkgCompiler *pkc = array_get_index(packages->values, i);
        for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
            FileCompiler *fc = array_get_index(pkc->file_compilers->values, o);

            for (int x = 0; x < fc->globals->length; x++) {
                GlobalVar *gv = array_get_index(fc->globals, x);
                if (gv->type == gv_threaded) {
                    str_append_chars(code, "pthread_key_create(&");
                    str_append_chars(code, gv->cname);
                    str_append_chars(code, ", (void*)0);\n");
                } else if (gv->type == gv_shared) {
                    str_append_chars(code, gv->cname);
                    str_append_chars(code, " = (void*)0;\n");
                }
            }

            for (int x = 0; x < fc->strings->length; x++) {
                ValueString *vstr = array_get_index(fc->strings, x);

                char *gname = vstr->name;

                str_append_chars(code, gname);
                str_append_chars(code, " = ki__type__String__make(\"");
                char *str = vstr->body;
                str_append_chars(code, str);
                str_append_chars(code, "\", ");
                size_t len = strlen(str);
                int count = 0;
                int diff = 0;
                char ch = '\0';
                char pch = '\0';
                // Dont count backslashes
                while (count < len) {
                    pch = ch;
                    ch = str[count];
                    if (pch == '\\') {
                        diff++;
                        count++;
                        if (count < len) {
                            ch = str[count];
                        }
                    }
                    count++;
                }

                len -= diff;
                char lenstr[20];
                sprintf(lenstr, "%zu", len);
                str_append_chars(code, lenstr);
                str_append_chars(code, ", 1);\n");
                // Keep in memory
                str_append_chars(code, gname);
                str_append_chars(code, "->_RC++;\n");
            }
        }
    }

    str_append_chars(code, "}\n");

    //
    char *code_ = str_to_chars(code);
    char *path = malloc(KI_PATH_MAX);
    strcpy(path, cache_dir);
    strcat(path, "/inits.c");
    write_file(path, code_, false);

    free(code_);
    free_str(code);
}

void fc_write_c(FileCompiler *fc) {
    // Write c + o file
    char *hcode = str_to_chars(fc->h_code);
    char *code = str_to_chars(fc->c_code);
    char *code_gen = str_to_chars(fc->c_code_after);

    // printf("code:\n");
    // printf("%s\n", code);

    fc->create_o_file = false;
    if (strlen(code) > 0 || strlen(code_gen) > 0) {
        fc->create_o_file = true;
        if (true) {
            write_file(fc->c_filepath, "\n#include \"project.h\"\n\n", false);
            write_file(fc->c_filepath, "int errno;\n\n", true);

            // char* incl = malloc(KI_PATH_MAX + 50);
            // for (int i = 0; i < fc->include_headers_from->length; i++) {
            //   FileCompiler* hfc = array_get_index(fc->include_headers_from, i);
            //   strcpy(incl, "#include \"");
            //   strcat(incl, hfc->h_filepath);
            //   strcat(incl, "\"\n");
            //   write_file(fc->c_filepath, incl, true);
            // }
            // free(incl);

            write_file(fc->c_filepath, "\n", true);
            if (!fc->is_header) {
                write_file(fc->c_filepath, code, true);
            }
            write_file(fc->c_filepath, code_gen, true);
        }

        array_push(o_files, fc->o_filepath);
    }

    if (fc->is_header && false) {
        write_file(fc->h_filepath, hcode, false);
    } else {
        char *path = malloc(KI_PATH_MAX);
        char *cache_dir = get_cache_dir();
        strcpy(path, cache_dir);
        strcat(path, "/project.h");
        write_file(path, hcode, true);
    }

    free(hcode);
    free(code);
    free(code_gen);
}

void fc_write_c_predefine_class(FileCompiler *fc, Class *class) {
    if (class->is_ctype) {
        str_append_chars(fc->struct_code, "typedef struct ");
        str_append_chars(fc->struct_code, class->cname);
        str_append_chars(fc->struct_code, " ");
        str_append_chars(fc->struct_code, class->cname);
        str_append_chars(fc->struct_code, ";\n");
    }

    str_append_chars(fc->struct_code, "struct ");
    str_append_chars(fc->struct_code, class->cname);
    str_append_chars(fc->struct_code, ";\n");
}

void fc_write_c_class(FileCompiler *fc, Class *class) {
    //
    str_append_chars(fc->h_code, "struct ");
    str_append_chars(fc->h_code, class->cname);
    str_append_chars(fc->h_code, " {\n");
    for (int i = 0; i < class->props->keys->length; i++) {
        char *name = array_get_index(class->props->keys, i);
        ClassProp *prop = array_get_index(class->props->values, i);
        if (prop->is_func) {
            continue;
        }
        if (prop->is_static) {
            continue;
        }
        fc_write_c_type(fc->h_code, prop->return_type, name);
        str_append_chars(fc->h_code, ";\n");
    }
    str_append_chars(fc->h_code, "};\n");

    // Free func
    ClassProp *prop = map_get(class->props, "__free");
    if (!prop || prop->generate_code == false) {
        str_append_chars(fc->h_code, "void ");
        str_append_chars(fc->h_code, class->cname);
        str_append_chars(fc->h_code, "____free(struct ");
        str_append_chars(fc->h_code, class->cname);
        str_append_chars(fc->h_code, "* this);\n");

        str_append_chars(fc->c_code, "void ");
        str_append_chars(fc->c_code, class->cname);
        str_append_chars(fc->c_code, "____free(struct ");
        str_append_chars(fc->c_code, class->cname);
        str_append_chars(fc->c_code, "* this){\n");

        for (int i = 0; i < class->props->keys->length; i++) {
            char *name = array_get_index(class->props->keys, i);
            ClassProp *prop = array_get_index(class->props->values, i);
            if (prop->is_func) {
                continue;
            }
            if (prop->is_static) {
                continue;
            }

            if (prop->return_type->class && prop->return_type->class->ref_count) {
                bool nullable = prop->return_type->nullable;
                if (nullable) {
                    str_append_chars(fc->c_code, "if(this->");
                    str_append_chars(fc->c_code, name);
                    str_append_chars(fc->c_code, ") {\n");
                }
                str_append_chars(fc->c_code, "if(--this->");
                str_append_chars(fc->c_code, name);
                str_append_chars(fc->c_code, "->_RC == 0) ");
                str_append_chars(fc->c_code, prop->return_type->class->cname);
                str_append_chars(fc->c_code, "____free(this->");
                str_append_chars(fc->c_code, name);
                str_append_chars(fc->c_code, ");\n");
                if (nullable) {
                    str_append_chars(fc->c_code, "}\n");
                }
            }
        }

        char *alloc_func = fc_write_c_get_allocator(fc, class->size, true);

        str_append_chars(fc->c_code, "ki__mem__Allocator__free(");
        str_append_chars(fc->c_code, alloc_func);
        str_append_chars(fc->c_code, "(), this);\n");
        str_append_chars(fc->c_code, "}\n\n");
    }
}

void fc_write_c_enum(FileCompiler *fc, Enum *enu) {
    //
    str_append_chars(fc->h_code, "typedef enum ");
    str_append_chars(fc->h_code, enu->cname);
    str_append_chars(fc->h_code, "{\n");

    int len = enu->values->keys->length;
    for (int i = 0; i < len; i++) {
        char *name = array_get_index(enu->values->keys, i);
        char *value = array_get_index(enu->values->values, i);
        str_append_chars(fc->h_code, name);
        str_append_chars(fc->h_code, " = ");
        str_append_chars(fc->h_code, value);
        str_append_chars(fc->h_code, ",\n");
    }

    str_append_chars(fc->h_code, "} ");
    str_append_chars(fc->h_code, enu->cname);
    str_append_chars(fc->h_code, ";\n");
}

void fc_write_c_global(FileCompiler *fc, GlobalVar *gv) {
    //
    if (gv->type == gv_threaded) {
        str_append_chars(fc->h_code, "struct pthread_key_t ");
        str_append_chars(fc->h_code, gv->cname);
        str_append_chars(fc->h_code, ";\n");
    } else if (gv->type == gv_shared) {
        fc_write_c_type(fc->h_code, gv->return_type, gv->cname);
        str_append_chars(fc->h_code, ";\n");
    }
}

void fc_write_c_func(FileCompiler *fc, Function *func) {

    //
    fc_write_c_type(fc->tkn_buffer, func->return_type, NULL);
    fc_write_c_type(fc->h_code, func->return_type, NULL);
    str_append_chars(fc->tkn_buffer, " ");
    str_append_chars(fc->h_code, " ");
    str_append_chars(fc->tkn_buffer, func->cname);
    str_append_chars(fc->h_code, func->cname);
    str_append_chars(fc->tkn_buffer, "(");
    str_append_chars(fc->h_code, "(");
    // Write args
    int arg_len = func->args->length;
    int x = 0;
    while (x < arg_len) {
        FunctionArg *arg = array_get_index(func->args, x);
        char *name = arg->name;
        Type *type = arg->type;
        fc_write_c_type(fc->tkn_buffer, type, name);
        fc_write_c_type(fc->h_code, type, name);
        x++;
        if (x < arg_len) {
            str_append_chars(fc->tkn_buffer, ", ");
            str_append_chars(fc->h_code, ", ");
        }
    }

    if (func->can_error) {
        if (arg_len > 0) {
            str_append_chars(fc->tkn_buffer, ", ");
            str_append_chars(fc->h_code, ", ");
        }
        str_append_chars(fc->tkn_buffer, "char** _KI_THROW_MSG");
        str_append_chars(fc->h_code, "char** _KI_THROW_MSG");
    }
    //
    str_append_chars(fc->h_code, ");\n");

    if (!fc->is_header) {
        str_append_chars(fc->tkn_buffer, ") {\n");
        fc->indent++;

        if (func->scope->catch_errors) {
            str_append_chars(fc->tkn_buffer, "char* _KI_THROW_MSG_BUF = 0;\n");
        }

        // If we want to assign new values to argument variables, this code is required
        // Currently we dont allow this, because of performance (extra ref counting)
        // int x = 0;
        // while (x < arg_len) {
        //     FunctionArg *arg = array_get_index(func->args, x);
        //     char *name = arg->name;
        //     Type *type = arg->type;
        //     x++;
        //     Class *class = type->class;
        //     if (class && class->ref_count) {
        //         if (type->nullable) {
        //             str_append_chars(fc->tkn_buffer, "if(");
        //             str_append_chars(fc->tkn_buffer, name);
        //             str_append_chars(fc->tkn_buffer, ") ");
        //         }
        //         str_append_chars(fc->tkn_buffer, name);
        //         str_append_chars(fc->tkn_buffer, "->_RC++;\n");
        //     }
        // }

        if (strcmp(func->cname, "main") == 0) {
            str_append_chars(fc->tkn_buffer, "KI_ALLOCATORS = ki__mem__calloc_flat(64 * 8);\n");
            str_append_chars(fc->tkn_buffer, "KI_ALLOCATORS_MUT = ki__async__Mutex__make();\n");
            str_append_chars(fc->tkn_buffer, "KI_INITS();\n");
        }
        if (uses_async && strcmp(func->cname, "main") == 0) {
            str_append_chars(fc->tkn_buffer, "void* KI_MAIN_TMS = ki__async__Taskman__setup_task_managers();\n");
        }

        // Body
        fc_write_c_ast(fc, func->scope);

        if (uses_async && strcmp(func->cname, "main") == 0) {
            str_append_chars(fc->tkn_buffer, "ki__async__Taskman__wait_for_tasks_to_end(KI_MAIN_TMS);\n");
        }

        fc->indent--;
        str_append_chars(fc->tkn_buffer, "}\n\n");
    }
}

void fc_write_c_ast(FileCompiler *fc, Scope *scope) {
    Scope *prev_scope = fc->current_scope;
    fc->current_scope = scope;

    int c = 0;
    Array *ast = scope->ast;
    while (c < ast->length) {
        Token *t = array_get_index(ast, c);
        fc_write_c_token(fc, t);
        c++;
    }

    if (!scope->did_return) {
        deref_local_vars(fc, NULL, fc->current_scope);
    }

    fc->current_scope = prev_scope;
}

void fc_write_c_token(FileCompiler *fc, Token *token) {
    Str *prev_buf = fc->tkn_buffer;
    Str *prev_before_buf = fc->before_tkn_buffer;
    Str *buf = str_make("");
    Str *before_buf = str_make("");
    fc->tkn_buffer = buf;
    fc->before_tkn_buffer = before_buf;
    //
    // printf("tt:%d\n", token->type);
    fc_indent(fc, fc->tkn_buffer);
    if (token->type == tkn_func) {
        fc_write_c_func(fc, token->item);
    } else if (token->type == tkn_debug_msg) {
        char *msg = token->item;
        str_append_chars(fc->tkn_buffer, "write(1, \"");
        str_append_chars(fc->tkn_buffer, msg);
        str_append_chars(fc->tkn_buffer, "\\n\", ");
        sprintf(fc->sprintf, "%ld", strlen(msg) + 1);
        str_append_chars(fc->tkn_buffer, fc->sprintf);
        str_append_chars(fc->tkn_buffer, ");\n");
        str_append_chars(fc->tkn_buffer, "write(1, \"errno:\", 6);\n");
        str_append_chars(fc->tkn_buffer, "write(1, &errno, 4);\n");
        str_append_chars(fc->tkn_buffer, "write(1, \"\\n\", 1);\n");
    } else if (token->type == tkn_exit) {
        ErrorToken *err = token->item;
        str_append_chars(fc->tkn_buffer, "ki__sys__exit(");
        str_append_chars(fc->tkn_buffer, err->msg);
        str_append_chars(fc->tkn_buffer, ");\n");
    } else if (token->type == tkn_panic) {
        ErrorToken *err = token->item;
        str_append_chars(fc->tkn_buffer, "write(1, \"");
        str_append_chars(fc->tkn_buffer, err->msg);
        str_append_chars(fc->tkn_buffer, "\", ");
        sprintf(fc->sprintf, "%ld", strlen(err->msg));
        str_append_chars(fc->tkn_buffer, fc->sprintf);
        str_append_chars(fc->tkn_buffer, ");\n");
        str_append_chars(fc->tkn_buffer, "ki__sys__exit(1);\n");
    } else if (token->type == tkn_each) {
        TokenEach *te = token->item;
        fc_write_c_value(fc, te->value, true);
        Class *class = te->value->return_type->class;
        ClassProp *fcountp = map_get(class->props, "__each_count");
        ClassProp *fgetp = map_get(class->props, "__each_get");
        Function *fcount = fcountp->func;
        Function *fget = fgetp->func;

        char *buf_count_name = strdup(var_buf(fc));
        char *buf_max_name = strdup(var_buf(fc));
        char *buf_error_name = strdup(var_buf(fc));
        str_append_chars(fc->tkn_buffer, "unsigned long int ");
        str_append_chars(fc->tkn_buffer, buf_count_name);
        str_append_chars(fc->tkn_buffer, " = 0;\n");
        str_append_chars(fc->tkn_buffer, "unsigned long int ");
        str_append_chars(fc->tkn_buffer, buf_max_name);
        str_append_chars(fc->tkn_buffer, " = ");
        str_append_chars(fc->tkn_buffer, fcount->cname);
        str_append_chars(fc->tkn_buffer, "(");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ");\n");
        // While loop
        str_append_chars(fc->tkn_buffer, "while(");
        str_append_chars(fc->tkn_buffer, buf_count_name);
        str_append_chars(fc->tkn_buffer, " < ");
        str_append_chars(fc->tkn_buffer, buf_max_name);
        str_append_chars(fc->tkn_buffer, "){\n");
        // Get item
        str_append_chars(fc->tkn_buffer, "char* ");
        str_append_chars(fc->tkn_buffer, buf_error_name);
        str_append_chars(fc->tkn_buffer, " = (void*)0;");

        fc_write_c_type(fc->tkn_buffer, fget->return_type, te->vvar->gen_name);
        str_append_chars(fc->tkn_buffer, " = ");
        str_append_chars(fc->tkn_buffer, fget->cname);
        str_append_chars(fc->tkn_buffer, "(");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ",");
        str_append_chars(fc->tkn_buffer, buf_count_name);
        str_append_chars(fc->tkn_buffer, ", &");
        str_append_chars(fc->tkn_buffer, buf_error_name);
        str_append_chars(fc->tkn_buffer, ");\n");
        //
        if (te->kvar) {
            str_append_chars(fc->tkn_buffer, "unsigned long int ");
            str_append_chars(fc->tkn_buffer, te->kvar->gen_name);
            str_append_chars(fc->tkn_buffer, " = ");
            str_append_chars(fc->tkn_buffer, buf_count_name);
            str_append_chars(fc->tkn_buffer, ";\n");
        }
        // Increase index
        str_append_chars(fc->tkn_buffer, buf_count_name);
        str_append_chars(fc->tkn_buffer, "++;\n");
        // Check error
        str_append_chars(fc->tkn_buffer, "if(");
        str_append_chars(fc->tkn_buffer, buf_error_name);
        str_append_chars(fc->tkn_buffer, "){ continue; }\n");

        // Scope AST
        fc_write_c_ast(fc, te->scope);

        str_append_chars(fc->tkn_buffer, "}\n");

        free(buf_count_name);
        free(buf_max_name);
        free(buf_error_name);

    } else if (token->type == tkn_declare) {
        TokenDeclare *decl = token->item;

        fc_write_c_value(fc, decl->value, true);

        fc_write_c_type(fc->tkn_buffer, decl->type, decl->name);
        str_append_chars(fc->tkn_buffer, " = ");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ";\n");

        Class *class = decl->value->return_type->class;
        bool nullable = decl->value->return_type->nullable;
        if (class && class->ref_count) {
            if (nullable) {
                str_append_chars(fc->tkn_buffer, "if(");
                str_append_chars(fc->tkn_buffer, decl->name);
                str_append_chars(fc->tkn_buffer, ") ");
            }
            str_append_chars(fc->tkn_buffer, decl->name);
            str_append_chars(fc->tkn_buffer, "->_RC++;\n");

            array_push(fc->current_scope->local_var_names, decl);
        }
    } else if (token->type == tkn_assign) {
        TokenAssign *ta = token->item;

        char *left;
        if (ta->left->type == vt_threaded_global) {
            GlobalVar *gv = ta->left->item;
            left = strdup(var_buf(fc));
            fc_write_c_type(fc->tkn_buffer, ta->left->return_type, left);
            str_append_chars(fc->tkn_buffer, " = 0;\n");
        } else {
            fc_write_c_value(fc, ta->left, true);
            left = str_to_chars(fc->value_buffer);
        }
        fc_write_c_value(fc, ta->right, true);

        bool lrefc = false;
        bool lrefc_nullable = false;
        bool rrefc = false;
        bool rrefc_nullable = false;
        Class *class = NULL;
        if (ta->type == op_eq) {
            Value *left = ta->left;
            Value *right = ta->right;
            class = left->return_type->class;
            Class *rclass = right->return_type->class;

            if (class && class->ref_count) {
                lrefc = true;
                if (left->return_type->nullable) {
                    lrefc_nullable = true;
                }
            }
            if (rclass && rclass->ref_count) {
                rrefc = true;
                if (right->return_type->nullable) {
                    rrefc_nullable = true;
                }
            }
        }

        // RC++ the new value first
        if (rrefc) {
            if (rrefc_nullable) {
                str_append_chars(fc->tkn_buffer, "if(");
                str_append(fc->tkn_buffer, fc->value_buffer);
                str_append_chars(fc->tkn_buffer, "){ ");
            }
            str_append(fc->tkn_buffer, fc->value_buffer);
            str_append_chars(fc->tkn_buffer, "->_RC++;");
            if (rrefc_nullable) {
                str_append_chars(fc->tkn_buffer, " }");
            }
            str_append_chars(fc->tkn_buffer, "\n");
        }

        // RC-- the old value
        if (lrefc) {
            if (lrefc_nullable) {
                str_append_chars(fc->tkn_buffer, "if(");
                str_append_chars(fc->tkn_buffer, left);
                str_append_chars(fc->tkn_buffer, "){ ");
            }
            str_append_chars(fc->tkn_buffer, left);
            str_append_chars(fc->tkn_buffer, "->_RC--;\n");
            str_append_chars(fc->tkn_buffer, "if(");
            str_append_chars(fc->tkn_buffer, left);
            str_append_chars(fc->tkn_buffer, "->_RC == 0) ");
            str_append_chars(fc->tkn_buffer, class->cname);
            str_append_chars(fc->tkn_buffer, "____free(");
            str_append_chars(fc->tkn_buffer, left);
            str_append_chars(fc->tkn_buffer, ");");
            if (lrefc_nullable) {
                str_append_chars(fc->tkn_buffer, " }");
            }
            str_append_chars(fc->tkn_buffer, "\n");
        }

        str_append_chars(fc->tkn_buffer, left);

        if (ta->type == op_eq) {
            str_append_chars(fc->tkn_buffer, " = ");
        } else if (ta->type == op_add) {
            str_append_chars(fc->tkn_buffer, " += ");
        } else if (ta->type == op_sub) {
            str_append_chars(fc->tkn_buffer, " -= ");
        } else if (ta->type == op_mult) {
            str_append_chars(fc->tkn_buffer, " *= ");
        } else if (ta->type == op_div) {
            str_append_chars(fc->tkn_buffer, " /= ");
        } else if (ta->type == op_mod) {
            str_append_chars(fc->tkn_buffer, " \%= ");
        } else if (ta->type == op_bit_OR) {
            str_append_chars(fc->tkn_buffer, " |= ");
        } else if (ta->type == op_bit_AND) {
            str_append_chars(fc->tkn_buffer, " &= ");
        } else if (ta->type == op_bit_XOR) {
            str_append_chars(fc->tkn_buffer, " ^= ");
        } else {
            fc_error(fc, "Unhandled assign operator translation", NULL);
        }

        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ";\n");

        if (ta->left->type == vt_threaded_global) {
            GlobalVar *gv = ta->left->item;
            str_append_chars(fc->tkn_buffer, "pthread_setspecific(");
            str_append_chars(fc->tkn_buffer, gv->cname);
            str_append_chars(fc->tkn_buffer, ", ");
            str_append_chars(fc->tkn_buffer, left);
            str_append_chars(fc->tkn_buffer, ");\n");
        }

        free(left);

    } else if (token->type == tkn_set_vscope_value) {

        TokenSetVscopeValue *sv = token->item;
        fc_write_c_value(fc, sv->value, true);

        deref_local_vars(fc, sv->value, sv->vscope);
        //
        str_append_chars(fc->tkn_buffer, sv->vname);
        str_append_chars(fc->tkn_buffer, " = ");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ";\n");

        str_append_chars(fc->tkn_buffer, "goto ");
        str_append_chars(fc->tkn_buffer, sv->vname);
        str_append_chars(fc->tkn_buffer, "_GOTO;\n");

    } else if (token->type == tkn_return) {
        Value *retv = NULL;
        if (token->item) {
            retv = token->item;
            fc_write_c_value(fc, token->item, true);
        }

        // Deref local vars + Check if var_bufs RC == 0 (if so free)
        deref_local_vars(fc, retv, NULL);

        //
        str_append_chars(fc->tkn_buffer, "return ");
        if (retv) {
            str_append(fc->tkn_buffer, fc->value_buffer);
        }
        str_append_chars(fc->tkn_buffer, ";\n");
    } else if (token->type == tkn_if) {
        fc_write_c_if(fc, token->item);
    } else if (token->type == tkn_ifnull) {
        //
        TokenIfNull *ifn = token->item;

        char *left = ifn->name;

        str_append_chars(fc->tkn_buffer, "if(");
        str_append_chars(fc->tkn_buffer, left);
        str_append_chars(fc->tkn_buffer, " == (void*)0) {\n");

        char *buf = fc_write_c_ort(fc, ifn->ort);

        if (buf) {
            str_append_chars(fc->tkn_buffer, left);
            str_append_chars(fc->tkn_buffer, " = ");
            str_append_chars(fc->tkn_buffer, buf);
            str_append_chars(fc->tkn_buffer, ";\n");
            free(buf);

            LocalVar *lv = ifn->idf->item;
            Type *type = lv->type;

            if (type->class && type->class->ref_count) {
                str_append_chars(fc->tkn_buffer, left);
                str_append_chars(fc->tkn_buffer, "->_RC++;\n");
            }
        }

        str_append_chars(fc->tkn_buffer, " }\n");
        //
    } else if (token->type == tkn_notnull) {
        //
        TokenNotNull *inn = token->item;
        str_append_chars(fc->tkn_buffer, "if(");
        str_append_chars(fc->tkn_buffer, inn->name);
        str_append_chars(fc->tkn_buffer, " != (void*)0) {\n");

        if (inn->type == or_do) {
            fc_write_c_ast(fc, inn->scope);
        }

        str_append_chars(fc->tkn_buffer, " }\n");
    } else if (token->type == tkn_while) {
        //
        TokenWhile *wt = token->item;
        fc_write_c_value(fc, wt->condition, true);
        str_append_chars(fc->tkn_buffer, "while(");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ") {\n");
        fc_write_c_ast(fc, wt->scope);
        str_append_chars(fc->tkn_buffer, "}\n\n");
    } else if (token->type == tkn_break) {
        Scope *scope = fc->current_scope;
        scope = get_loop_scope(scope);
        deref_local_vars(fc, NULL, scope);
        str_append_chars(fc->tkn_buffer, "break;\n");
    } else if (token->type == tkn_continue) {
        Scope *scope = fc->current_scope;
        scope = get_loop_scope(scope);
        deref_local_vars(fc, NULL, scope);
        str_append_chars(fc->tkn_buffer, "continue;\n");
    } else if (token->type == tkn_throw) {
        TokenThrow *tt = token->item;
        str_append_chars(fc->tkn_buffer, "*_KI_THROW_MSG = \"");
        str_append_chars(fc->tkn_buffer, tt->msg);
        str_append_chars(fc->tkn_buffer, "\";\n");
        if (tt->return_type == NULL) {
            str_append_chars(fc->tkn_buffer, "return;\n");
        } else if (tt->return_type->is_pointer) {
            str_append_chars(fc->tkn_buffer, "return (void*)0;\n");
        } else {
            str_append_chars(fc->tkn_buffer, "return 0;\n");
        }

    } else if (token->type == tkn_free) {
        fc_write_c_value(fc, token->item, true);
        str_append_chars(fc->tkn_buffer, "ki__mem__free(");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ");\n");
    } else if (token->type == tkn_value) {
        Value *val = token->item;
        fc_write_c_value(fc, token->item, true);
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ";\n");
    } else {
        printf("Token: %d\n", token->type);
        fc_error(fc, "Unhandled token", NULL);
    }

    //
    str_append(before_buf, buf);
    free_str(buf);
    if (prev_buf == NULL) {
        str_append(fc->c_code, before_buf);
    } else {
        str_append(prev_buf, before_buf);
    }
    free_str(before_buf);
    //
    fc->tkn_buffer = prev_buf;
    fc->before_tkn_buffer = prev_before_buf;
}

char *fc_write_c_ort(FileCompiler *fc, OrToken *ort) {
    //
    char *buf = NULL;
    if (ort->type == or_value) {
        buf = strdup(var_buf(fc));
        fc_write_c_type(fc->tkn_buffer, ort->primary_type, buf);
        str_append_chars(fc->tkn_buffer, ";\n");
    }

    if (ort->type == or_value) {
        if (ort->vscope) {
            fc_write_c_type(fc->tkn_buffer, ort->vscope->vscope_return_type, ort->vscope->vscope_vname);
            str_append_chars(fc->tkn_buffer, ";\n");
        }
    }

    str_append_chars(fc->tkn_buffer, "if(1){");

    if (ort->type == or_value) {
        if (ort->vscope) {
            fc_write_c_ast(fc, ort->vscope);
        } else {
            fc_write_c_value(fc, ort->value, true);
            str_append_chars(fc->tkn_buffer, buf);
            str_append_chars(fc->tkn_buffer, " = ");
            str_append(fc->tkn_buffer, fc->value_buffer);
            str_append_chars(fc->tkn_buffer, ";\n");
        }
    } else if (ort->type == or_return) {

        if (ort->vscope) {
            fc_write_c_ast(fc, ort->vscope);
        } else {
            if (ort->value) {
                fc_write_c_value(fc, ort->value, true);
            }
            Scope *fscope = get_func_scope(fc->current_scope);
            deref_local_vars(fc, ort->value, fscope);
            str_append_chars(fc->tkn_buffer, "return ");
            if (ort->value) {
                str_append(fc->tkn_buffer, fc->value_buffer);
            }
            str_append_chars(fc->tkn_buffer, ";\n");
        }
    } else if (ort->type == or_exit) {
        str_append_chars(fc->tkn_buffer, "exit(1);\n");
    } else if (ort->type == or_panic) {
        str_append_chars(fc->tkn_buffer, "exit(1);\n");
    } else if (ort->type == or_throw) {
        str_append_chars(fc->tkn_buffer, "*_KI_THROW_MSG = \"");
        str_append_chars(fc->tkn_buffer, ort->error->msg);
        str_append_chars(fc->tkn_buffer, "\";\n");
        Scope *scope = fc->current_scope;
        scope = get_func_scope(scope);
        if (scope->func->return_type == NULL) {
            str_append_chars(fc->tkn_buffer, "return;\n");
        } else if (scope->func->return_type->is_pointer) {
            str_append_chars(fc->tkn_buffer, "return (void*)0;\n");
        } else {
            str_append_chars(fc->tkn_buffer, "return 0;\n");
        }
    } else if (ort->type == or_break) {
        Scope *scope = get_loop_scope(fc->current_scope);
        deref_local_vars(fc, NULL, scope);
        str_append_chars(fc->tkn_buffer, "break;\n");
    } else if (ort->type == or_continue) {
        Scope *scope = get_loop_scope(fc->current_scope);
        deref_local_vars(fc, NULL, scope);
        str_append_chars(fc->tkn_buffer, "continue;\n");
    }

    str_append_chars(fc->tkn_buffer, " }\n");

    if (ort->type == or_value) {
        if (ort->vscope) {
            str_append_chars(fc->tkn_buffer, ort->vscope->vscope_vname);
            str_append_chars(fc->tkn_buffer, "_GOTO:\n");

            str_append_chars(fc->tkn_buffer, buf);
            str_append_chars(fc->tkn_buffer, " = ");
            str_append_chars(fc->tkn_buffer, ort->vscope->vscope_vname);
            str_append_chars(fc->tkn_buffer, ";\n");
        }
    }

    return buf;
}

char *indenter;
void fc_indent(FileCompiler *fc, Str *append_to) {
    //
    int chars = fc->indent * 2;
    if (chars > 0) {
        if (indenter == NULL) {
            int max_size = 1000;
            indenter = malloc(max_size);
            int c = 0;
            while (c < max_size) {
                indenter[c] = ' ';
                c++;
            }
            indenter[max_size - 1] = '\0';
        }
        indenter[chars] = '\0';
        str_append_chars(append_to, indenter);
        indenter[chars] = ' ';
    }
}

void fc_write_c_value(FileCompiler *fc, Value *value, bool new_value) {
    Str *result = fc->value_buffer;

    if (new_value) {
        result->length = 0;
    }

    //
    // printf("Type: %d\n", value->type);

    if (value->type == vt_null) {
        str_append_chars(result, "(void*)0");
        // Bools
    } else if (value->type == vt_false) {
        str_append_chars(result, "0");
    } else if (value->type == vt_true) {
        str_append_chars(result, "1");
    } else if (value->type == vt_group) {
        str_append_chars(result, "(");
        fc_write_c_value(fc, value->item, false);
        str_append_chars(result, ")");
    } else if (value->type == vt_var) {
        str_append_chars(result, value->item);
    } else if (value->type == vt_threaded_global) {
        //
        GlobalVar *gv = value->item;

        str_append_chars(result, "pthread_getspecific(");
        str_append_chars(result, gv->cname);
        str_append_chars(result, ")");
        //
    } else if (value->type == vt_shared_global) {
        //
        GlobalVar *gv = value->item;
        str_append_chars(result, gv->cname);
        //
    } else if (value->type == vt_arg) {
        str_append_chars(result, value->item);
    } else if (value->type == vt_number) {
        str_append_chars(result, value->item);
    } else if (value->type == vt_char) {
        str_append_chars(result, "'");
        str_append_chars(result, value->item);
        str_append_chars(result, "'");
    } else if (value->type == vt_operator) {
        ValueOperator *op = value->item;
        fc_write_c_value(fc, op->left, false);
        if (op->type == op_add) {
            str_append_chars(result, " + ");
        } else if (op->type == op_sub) {
            str_append_chars(result, " - ");
        } else if (op->type == op_mult) {
            str_append_chars(result, " * ");
        } else if (op->type == op_div) {
            str_append_chars(result, " / ");
        } else if (op->type == op_mod) {
            str_append_chars(result, " \% ");
        } else if (op->type == op_bit_OR) {
            str_append_chars(result, " | ");
        } else if (op->type == op_bit_AND) {
            str_append_chars(result, " & ");
        } else if (op->type == op_bit_XOR) {
            str_append_chars(result, " ^ ");
        } else if (op->type == op_bit_shift_left) {
            str_append_chars(result, "<<");
        } else if (op->type == op_bit_shift_right) {
            str_append_chars(result, ">>");
            //
        } else if (op->type == op_and) {
            str_append_chars(result, " && ");
        } else if (op->type == op_or) {
            str_append_chars(result, " || ");
            //
        } else if (op->type == op_eq) {
            str_append_chars(result, " == ");
        } else if (op->type == op_neq) {
            str_append_chars(result, " != ");
        } else if (op->type == op_lt) {
            str_append_chars(result, " < ");
        } else if (op->type == op_lte) {
            str_append_chars(result, " <= ");
        } else if (op->type == op_gt) {
            str_append_chars(result, " > ");
        } else if (op->type == op_gte) {
            str_append_chars(result, " >= ");
        } else if (op->type == op_incr) {
            str_append_chars(result, "++");
        } else if (op->type == op_decr) {
            str_append_chars(result, "--");
        } else {
            printf("Op: %d\n", op->type);
            fc_error(fc, "Unhandled operator type", NULL);
        }
        if (op->right) {
            fc_write_c_value(fc, op->right, false);
        }
    } else if (value->type == vt_func_call) {
        ValueFuncCall *fa = value->item;

        // Arg values
        char *cache = str_to_chars(fc->value_buffer);
        Array *arg_strings = array_make(4);
        for (int i = 0; i < fa->arg_values->length; i++) {
            Value *v = array_get_index(fa->arg_values, i);
            fc->value_buffer->length = 0;
            fc_write_c_value(fc, v, false);
            array_push(arg_strings, str_to_chars(fc->value_buffer));
        }
        fc->value_buffer->length = 0;
        str_append_chars(fc->value_buffer, cache);
        free(cache);

        //
        fc_write_c_value(fc, fa->on, false);
        str_append_chars(result, "(");
        for (int i = 0; i < arg_strings->length; i++) {
            if (i > 0) {
                str_append_chars(result, ", ");
            }
            char *arg_str = array_get_index(arg_strings, i);
            str_append_chars(result, arg_str);
            free(arg_str);
        }
        free(arg_strings);

        if (fa->error_type != or_none) {
            if (fa->arg_values->length > 0) {
                str_append_chars(result, ", ");
            }
            str_append_chars(result, "&_KI_THROW_MSG_BUF");
        }
        str_append_chars(result, ")");

        if (fa->error_type != or_none) {
            char *buf_var_name = NULL;
            if (value->return_type) {
                buf_var_name = strdup(var_buf(fc));
                fc_write_c_type(fc->tkn_buffer, value->return_type, buf_var_name);
                str_append_chars(fc->tkn_buffer, " = ");
            }
            str_append(fc->tkn_buffer, result);
            str_append_chars(fc->tkn_buffer, ";\n");
            //
            result->length = 0;
            // Check error

            if (fa->error_type == or_value && fa->or_scope) {
                fc_write_c_type(fc->tkn_buffer, fa->or_scope->vscope_return_type, fa->or_scope->vscope_vname);
                str_append_chars(fc->tkn_buffer, ";\n");
            }

            str_append_chars(fc->tkn_buffer, "if(_KI_THROW_MSG_BUF){\n");
            if (fa->error_type != or_pass) {
                str_append_chars(fc->tkn_buffer, "_KI_THROW_MSG_BUF = (void*)0;\n");
            }
            //
            if (fa->error_type == or_pass) {
                str_append_chars(fc->tkn_buffer, "*_KI_THROW_MSG = _KI_THROW_MSG_BUF;\n");
                str_append_chars(fc->tkn_buffer, "_KI_THROW_MSG_BUF = (void*)0;\n");
                Type *rett = fa->func_scope->func->return_type;
                if (rett == NULL) {
                    str_append_chars(fc->tkn_buffer, "return;\n");
                } else if (rett->is_pointer) {
                    str_append_chars(fc->tkn_buffer, "return (void*)0;\n");
                } else {
                    str_append_chars(fc->tkn_buffer, "return 0;\n");
                }
            } else if (fa->error_type == or_return) {
                Value *orv = fa->or_value;
                //
                fc->current_scope = fa->or_scope;
                fc_write_c_value(fc, orv, false);
                Scope *fscope = get_func_scope(fc->current_scope);
                deref_local_vars(fc, orv, fscope);
                fc->current_scope = fa->or_scope->parent;
                //
                str_append_chars(fc->tkn_buffer, "return ");
                str_append(fc->tkn_buffer, fc->value_buffer);
                str_append_chars(fc->tkn_buffer, ";\n");
            } else if (fa->error_type == or_value) {
                if (fa->or_scope) {
                    str_append_chars(fc->tkn_buffer, "char* ");
                    str_append_chars(fc->tkn_buffer, fa->or_error_vn);
                    str_append_chars(fc->tkn_buffer, " = _KI_THROW_MSG_BUF;\n");

                    fc_write_c_ast(fc, fa->or_scope);

                    fc->current_scope = fa->or_scope;
                    deref_local_vars(fc, value, fa->or_scope);
                    fc->current_scope = fa->or_scope->parent;

                    str_append_chars(fc->tkn_buffer, fa->or_scope->vscope_vname);
                    str_append_chars(fc->tkn_buffer, "_GOTO:\n");

                    str_append_chars(fc->tkn_buffer, buf_var_name);
                    str_append_chars(fc->tkn_buffer, " = ");
                    str_append_chars(fc->tkn_buffer, fa->or_scope->vscope_vname);
                    str_append_chars(fc->tkn_buffer, ";\n");
                } else {
                    Value *orv = fa->or_value;
                    fc_write_c_value(fc, orv, false);
                    str_append_chars(fc->tkn_buffer, buf_var_name);
                    str_append_chars(fc->tkn_buffer, " = ");
                    str_append(fc->tkn_buffer, fc->value_buffer);
                    str_append_chars(fc->tkn_buffer, ";\n");
                }
            } else if (fa->error_type == or_throw) {
                str_append_chars(fc->tkn_buffer, "*_KI_THROW_MSG = \"");
                str_append_chars(fc->tkn_buffer, fa->throw_msg);
                str_append_chars(fc->tkn_buffer, "\";\n");
                Type *rett = fa->func_scope->func->return_type;
                if (rett == NULL) {
                    str_append_chars(fc->tkn_buffer, "return;\n");
                } else if (rett->is_pointer) {
                    str_append_chars(fc->tkn_buffer, "return (void*)0;\n");
                } else {
                    str_append_chars(fc->tkn_buffer, "return 0;\n");
                }
            }
            //
            str_append_chars(fc->tkn_buffer, "}\n");
            // Update result
            result->length = 0;
            if (value->return_type) {
                str_append_chars(result, buf_var_name);
                free(buf_var_name);
            }
        }

        if (value->return_type) {
            Class *retClass = value->return_type->class;
            if (retClass && retClass->ref_count) {
                // Buffer the value
                char *buf_var_name = strdup(var_buf(fc));
                str_append_chars(fc->tkn_buffer, "struct ");
                str_append_chars(fc->tkn_buffer, retClass->cname);
                str_append_chars(fc->tkn_buffer, "* ");
                str_append_chars(fc->tkn_buffer, buf_var_name);
                str_append_chars(fc->tkn_buffer, " = ");
                str_append(fc->tkn_buffer, result);
                str_append_chars(fc->tkn_buffer, ";\n");

                if (value->return_type->nullable) {
                    str_append_chars(fc->tkn_buffer, "if(");
                    str_append_chars(fc->tkn_buffer, buf_var_name);
                    str_append_chars(fc->tkn_buffer, ") ");
                }

                str_append_chars(fc->tkn_buffer, buf_var_name);
                str_append_chars(fc->tkn_buffer, "->_RC++;\n");
                result->length = 0;
                str_append_chars(result, buf_var_name);

                VarInfo *vi = malloc(sizeof(VarInfo));
                vi->name = buf_var_name;
                vi->return_type = value->return_type;

                array_push(fc->current_scope->var_bufs, vi);
            }
        }

    } else if (value->type == vt_sizeof) {
        str_append_chars(result, value->item);
    } else if (value->type == vt_cast) {
        ValueCast *cast = value->item;
        str_append_chars(result, "(");
        fc_write_c_type(result, cast->as_type, NULL);
        str_append_chars(result, ")");
        fc_write_c_value(fc, cast->value, false);
    } else if (value->type == vt_getptrv) {
        ValueCast *cast = value->item;
        str_append_chars(result, "*(");
        fc_write_c_type(result, cast->as_type, NULL);
        str_append_chars(result, "*)(");
        fc_write_c_value(fc, cast->value, false);
        str_append_chars(result, ")");
    } else if (value->type == vt_getptr) {
        str_append_chars(result, "&");
        fc_write_c_value(fc, value->item, false);
    } else if (value->type == vt_setptrv) {
        SetPtrValue *cast = value->item;
        str_append_chars(result, "*(");
        fc_write_c_type(result, cast->to_value->return_type, NULL);
        str_append_chars(result, "*)");
        fc_write_c_value(fc, cast->ptr_value, false);
        str_append_chars(result, " = ");
        fc_write_c_value(fc, cast->to_value, false);
    } else if (value->type == vt_class_init) {
        // Generate function
        GEN_C++;
        ValueClassInit *ini = value->item;
        Class *class = ini->class;

        char *allocator_name = fc_write_c_get_allocator(fc, class->size, true);
        char *func_name = malloc(30);
        sprintf(func_name, "_KI_CLASS_INIT_%d", GEN_C);

        char *buf_var_name = strdup(var_buf(fc));
        str_append_chars(result, buf_var_name);

        // Set cache
        char *cache = str_to_chars(fc->value_buffer);

        //
        Str *args_str = str_make("");
        for (int i = 0; i < ini->prop_values->values->length; i++) {
            if (i > 0) {
                str_append_chars(args_str, ", ");
            }
            Value *val = array_get_index(ini->prop_values->values, i);
            fc->value_buffer->length = 0;
            fc_write_c_value(fc, val, false);
            str_append(args_str, fc->value_buffer);
        }
        fc_write_c_type(fc->tkn_buffer, value->return_type, buf_var_name);
        str_append_chars(fc->tkn_buffer, " = ");

        str_append_chars(fc->tkn_buffer, func_name);
        str_append_chars(fc->tkn_buffer, "(");
        str_append(fc->tkn_buffer, args_str);
        str_append_chars(fc->tkn_buffer, ");\n");
        free_str(args_str);

        // Restore cache
        fc->value_buffer->length = 0;
        str_append_chars(fc->value_buffer, cache);
        free(cache);

        // Write header
        fc_write_c_type(fc->h_code, value->return_type, NULL);
        str_append_chars(fc->h_code, " ");
        str_append_chars(fc->h_code, func_name);
        str_append_chars(fc->h_code, "(");
        for (int i = 0; i < ini->prop_values->keys->length; i++) {
            if (i > 0) {
                str_append_chars(fc->h_code, ", ");
            }
            char *prop_name = array_get_index(ini->prop_values->keys, i);
            ClassProp *prop = map_get(class->props, prop_name);
            fc_write_c_type(fc->h_code, prop->return_type, prop_name);
        }
        str_append_chars(fc->h_code, ");\n");

        // Write func
        fc_write_c_type(fc->c_code_after, value->return_type, NULL);
        str_append_chars(fc->c_code_after, " ");
        str_append_chars(fc->c_code_after, func_name);
        str_append_chars(fc->c_code_after, "(");
        for (int i = 0; i < ini->prop_values->keys->length; i++) {
            if (i > 0) {
                str_append_chars(fc->c_code_after, ", ");
            }
            char *prop_name = array_get_index(ini->prop_values->keys, i);
            ClassProp *prop = map_get(class->props, prop_name);
            fc_write_c_type(fc->c_code_after, prop->return_type, prop_name);
        }
        str_append_chars(fc->c_code_after, ") {\n");
        char *sign;
        if (value->return_type->is_pointer) {
            sign = "->";
            fc_write_c_type(fc->c_code_after, value->return_type, "KI_RET_V");

            str_append_chars(fc->c_code_after, " = ki__mem__Allocator__get_chunk(");
            str_append_chars(fc->c_code_after, allocator_name);
            str_append_chars(fc->c_code_after, "());\n");
        } else {
            sign = ".";
            fc_write_c_type(fc->c_code_after, value->return_type, "KI_RET_V");
            str_append_chars(fc->c_code_after, ";\n");
        }

        // if (class->ref_count) {
        //   str_append_chars(fc->c_code_after, "KI_RET_V");
        //   str_append_chars(fc->c_code_after, sign);
        //   str_append_chars(fc->c_code_after, "_RC = 0;\n");
        // }

        for (int i = 0; i < ini->prop_values->keys->length; i++) {
            char *prop_name = array_get_index(ini->prop_values->keys, i);
            Value *v = array_get_index(ini->prop_values->values, i);
            str_append_chars(fc->c_code_after, "KI_RET_V");
            str_append_chars(fc->c_code_after, sign);
            str_append_chars(fc->c_code_after, prop_name);
            str_append_chars(fc->c_code_after, " = ");
            str_append_chars(fc->c_code_after, prop_name);
            str_append_chars(fc->c_code_after, ";\n");

            if (v->return_type->class && v->return_type->class->ref_count) {
                str_append_chars(fc->c_code_after, "KI_RET_V");
                str_append_chars(fc->c_code_after, sign);
                str_append_chars(fc->c_code_after, prop_name);
                str_append_chars(fc->c_code_after, "->_RC++;\n");
            }
        }

        if (class->ref_count) {
            str_append_chars(fc->c_code_after, "KI_RET_V");
            str_append_chars(fc->c_code_after, sign);
            str_append_chars(fc->c_code_after, "_ALLOCATOR = ");
            str_append_chars(fc->c_code_after, allocator_name);
            str_append_chars(fc->c_code_after, "();\n");
        }

        Scope *prev_scope = fc->current_scope;
        fc->current_scope = init_scope();

        Str *prevbuf = fc->tkn_buffer;
        fc->tkn_buffer = str_make("");

        for (int i = 0; i < class->props->keys->length; i++) {
            char *prop_name = array_get_index(class->props->keys, i);
            if (map_contains(ini->prop_values, prop_name)) {
                continue;
            }
            ClassProp *prop = array_get_index(class->props->values, i);
            if (!prop->default_value) {
                continue;
            }

            char *cache = str_to_chars(fc->value_buffer);
            fc->value_buffer->length = 0;
            fc_write_c_value(fc, prop->default_value, false);
            char *defv = str_to_chars(fc->value_buffer);
            fc->value_buffer->length = 0;
            str_append_chars(fc->value_buffer, cache);
            free(cache);

            str_append_chars(fc->tkn_buffer, "KI_RET_V");
            str_append_chars(fc->tkn_buffer, sign);
            str_append_chars(fc->tkn_buffer, prop_name);
            str_append_chars(fc->tkn_buffer, " = ");
            str_append_chars(fc->tkn_buffer, defv);
            str_append_chars(fc->tkn_buffer, ";\n");

            free(defv);
        }

        deref_local_vars(fc, NULL, fc->current_scope);

        str_append(fc->c_code_after, fc->tkn_buffer);
        free_str(fc->tkn_buffer);
        fc->tkn_buffer = prevbuf;

        free_scope(fc->current_scope);
        fc->current_scope = prev_scope;

        str_append_chars(fc->c_code_after, "return KI_RET_V;\n");
        str_append_chars(fc->c_code_after, "}\n\n");

    } else if (value->type == vt_prop_access) {
        ValueClassPropAccess *pa = value->item;

        if (pa->is_static) {
            Class *class = pa->on;
            // ClassProp* prop = map_get(class->props, pa->name);
            //  func ref
            //  Type* type = prop->return_type;
            str_append_chars(result, class->cname);
            str_append_chars(result, "__");
            str_append_chars(result, pa->name);
        } else {
            Value *val = pa->on;
            fc_write_c_value(fc, pa->on, false);
            Type *type = val->return_type;
            if (type->is_pointer) {
                str_append_chars(result, "->");
            } else {
                str_append_chars(result, ".");
            }
            str_append_chars(result, pa->name);
        }
    } else if (value->type == vt_async) {
        Value *fcallv = value->item;
        ValueFuncCall *fcall = fcallv->item;
        Value *on = fcall->on;
        char *size = malloc(10);
        //
        Type *task_type = fc_identifier_to_type(fc, create_identifier("ki", "async", "Task"), NULL);
        // Cache current value
        char *cache = str_to_chars(fc->value_buffer);

        // Step 1. Generate execution function
        char *handler_name = strdup(var_buf(fc));
        str_append_chars(fc->c_code_after, "void ");
        str_append_chars(fc->c_code_after, handler_name);
        str_append_chars(fc->c_code_after, "(");
        fc_write_c_type(fc->c_code_after, task_type, "task");
        str_append_chars(fc->c_code_after, ") {\n");
        str_append_chars(fc->c_code_after, "void* arg_pointer = task->args;\n");
        // Header
        str_append_chars(fc->h_code, "void ");
        str_append_chars(fc->h_code, handler_name);
        str_append_chars(fc->h_code, "(");
        fc_write_c_type(fc->h_code, task_type, "task");
        str_append_chars(fc->h_code, ");\n");

        // Body
        int args_size = 0;
        Array *arg_strings = array_make(4);
        for (int i = 0; i < fcall->arg_values->length; i++) {
            char *arg_name = malloc(10);
            Value *v = array_get_index(fcall->arg_values, i);
            args_size += v->return_type->bytes;
            sprintf(arg_name, "arg_%d", i);
            array_push(arg_strings, arg_name);
            fc_write_c_type(fc->c_code_after, v->return_type, arg_name);
            str_append_chars(fc->c_code_after, " = *(");
            fc_write_c_type(fc->c_code_after, v->return_type, NULL);
            str_append_chars(fc->c_code_after, "*)arg_pointer;\n");
            str_append_chars(fc->c_code_after, "arg_pointer += ");
            sprintf(size, "%d", v->return_type->bytes);
            str_append_chars(fc->c_code_after, size);
            str_append_chars(fc->c_code_after, ";\n");
        }
        // Call func
        char *func_ref_name = strdup(var_buf(fc));
        str_append_chars(fc->c_code_after, "void* ");
        str_append_chars(fc->c_code_after, func_ref_name);
        str_append_chars(fc->c_code_after, " = task->func;\n");

        char *ret_name = NULL;
        if (fcallv->return_type) {
            ret_name = var_buf(fc);
            fc_write_c_type(fc->c_code_after, fcallv->return_type, ret_name);
            str_append_chars(fc->c_code_after, " = ");
        }
        str_append_chars(fc->c_code_after, "((");
        fc_write_c_type(fc->c_code_after, on->return_type, NULL);
        str_append_chars(fc->c_code_after, ")");
        str_append_chars(fc->c_code_after, func_ref_name);
        free(func_ref_name);
        str_append_chars(fc->c_code_after, ")(");
        // Args
        for (int i = 0; i < arg_strings->length; i++) {
            char *arg_name = array_get_index(arg_strings, i);
            if (i > 0) {
                str_append_chars(fc->c_code_after, ", ");
            }
            str_append_chars(fc->c_code_after, arg_name);
        }
        str_append_chars(fc->c_code_after, ");\n");
        // End func call
        if (ret_name) {
            str_append_chars(fc->c_code_after, "task->result = (void*)");
            str_append_chars(fc->c_code_after, ret_name);
            str_append_chars(fc->c_code_after, ";\n");
        }
        str_append_chars(fc->c_code_after, "task->ready = 1;\n");
        // Deref args if needed
        for (int i = 0; i < fcall->arg_values->length; i++) {
            Value *v = array_get_index(fcall->arg_values, i);
            char *arg_name = array_get_index(arg_strings, i);
            if (v->return_type->class && v->return_type->class->ref_count) {
                str_append_chars(fc->c_code_after, "if(--");
                str_append_chars(fc->c_code_after, arg_name);
                str_append_chars(fc->c_code_after, "->_RC == 0) ");
                str_append_chars(fc->c_code_after, v->return_type->class->cname);
                str_append_chars(fc->c_code_after, "____free(");
                str_append_chars(fc->c_code_after, arg_name);
                str_append_chars(fc->c_code_after, ");\n");
            }
        }
        array_free(arg_strings);
        // End body
        str_append_chars(fc->c_code_after, "}\n\n");

        // Step 2. Create Task and push onto stack
        // Func ref
        char *allocator_name = fc_write_c_get_allocator(fc, task_type->class->size, false);
        char *func_name = strdup(var_buf(fc));
        str_append_chars(fc->tkn_buffer, "void* ");
        str_append_chars(fc->tkn_buffer, func_name);
        str_append_chars(fc->tkn_buffer, " = ");
        fc->value_buffer->length = 0;
        fc_write_c_value(fc, on, false);
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ";\n");
        // Init Task
        char *var_name = strdup(var_buf(fc));
        fc_write_c_type(fc->tkn_buffer, task_type, var_name);
        str_append_chars(fc->tkn_buffer, " = ki__mem__Allocator__get_chunk(");
        str_append_chars(fc->tkn_buffer, allocator_name);
        str_append_chars(fc->tkn_buffer, "());\n");
        // str_append_chars(fc->tkn_buffer, " = ki__mem__alloc(");
        // sprintf(size, "%d", task_type->class->size);
        // str_append_chars(fc->tkn_buffer, size);
        // str_append_chars(fc->tkn_buffer, ");\n");
        //
        str_append_chars(fc->tkn_buffer, var_name);
        str_append_chars(fc->tkn_buffer, "->handler_func = ");
        str_append_chars(fc->tkn_buffer, handler_name);
        str_append_chars(fc->tkn_buffer, ";\n");
        // Set function
        str_append_chars(fc->tkn_buffer, var_name);
        str_append_chars(fc->tkn_buffer, "->func = ");
        str_append_chars(fc->tkn_buffer, func_name);
        str_append_chars(fc->tkn_buffer, ";\n");
        // Set args
        str_append_chars(fc->tkn_buffer, var_name);
        str_append_chars(fc->tkn_buffer, "->args = ki__mem__alloc_flat(");
        sprintf(size, "%d", args_size);
        str_append_chars(fc->tkn_buffer, size);
        str_append_chars(fc->tkn_buffer, ");\n");

        char *argsptr_name = var_buf(fc);
        str_append_chars(fc->tkn_buffer, "void* ");
        str_append_chars(fc->tkn_buffer, argsptr_name);
        str_append_chars(fc->tkn_buffer, " = ");
        str_append_chars(fc->tkn_buffer, var_name);
        str_append_chars(fc->tkn_buffer, "->args;\n");

        for (int i = 0; i < fcall->arg_values->length; i++) {
            Value *v = array_get_index(fcall->arg_values, i);
            fc->value_buffer->length = 0;
            fc_write_c_value(fc, v, false);
            //
            str_append_chars(fc->tkn_buffer, "*(");
            fc_write_c_type(fc->tkn_buffer, v->return_type, NULL);
            str_append_chars(fc->tkn_buffer, "*)");
            str_append_chars(fc->tkn_buffer, argsptr_name);
            str_append_chars(fc->tkn_buffer, " = ");
            str_append(fc->tkn_buffer, fc->value_buffer);
            str_append_chars(fc->tkn_buffer, ";\n");

            if (v->return_type->class && v->return_type->class->ref_count) {
                str_append(fc->tkn_buffer, fc->value_buffer);
                str_append_chars(fc->tkn_buffer, "->_RC++;\n");
            }

            str_append_chars(fc->tkn_buffer, argsptr_name);
            str_append_chars(fc->tkn_buffer, " += ");
            sprintf(size, "%d", v->return_type->bytes);
            str_append_chars(fc->tkn_buffer, size);
            str_append_chars(fc->tkn_buffer, ";\n");
        }

        // Push task on stack
        str_append_chars(fc->tkn_buffer, "ki__async__Taskman__add_task(");
        str_append_chars(fc->tkn_buffer, var_name);
        str_append_chars(fc->tkn_buffer, ");\n");

        // Set cache back
        fc->value_buffer->length = 0;
        str_append_chars(fc->value_buffer, cache);
        free(cache);
        free(size);
        free(func_name);
        free(handler_name);

        str_append_chars(result, var_name);

        // free(var_name);
    } else if (value->type == vt_await) {
        // loop until task is ready
        // if not ready, check for other tasks todo
        fc_write_c_value(fc, value->item, false);
        //
        str_append_chars(fc->tkn_buffer, "while(!");
        str_append(fc->tkn_buffer, result);
        str_append_chars(fc->tkn_buffer, "->ready){\n");
        str_append_chars(fc->tkn_buffer, "ki__async__Taskman__run_another_task();\n");
        str_append_chars(fc->tkn_buffer, "}\n");

        str_append_chars(result, "->result");
        //
    } else if (value->type == vt_allocator) {
        char *size = value->item;
        int sizei = atoi(size);
        char *name = fc_write_c_get_allocator(fc, sizei, true);
        str_append_chars(result, name);
        str_append_chars(result, "()");
    } else {
        fc_error(fc, "Unhandled value token (compiler bug)", NULL);
    }
}

char i_to_str_buf[100];

void fc_write_c_type_varname(Str *append_to, Type *type, char *varname) {
    if (varname) {
        if (type && type->type != type_funcref) {
            str_append_chars(append_to, " ");
        }
        str_append_chars(append_to, varname);
    }
    if (type && type->is_array) {
        str_append_chars(append_to, "[");
        sprintf(i_to_str_buf, "%d", type->array_size);
        str_append_chars(append_to, i_to_str_buf);
        str_append_chars(append_to, "]");
    }
}

void fc_write_c_type(Str *append_to, Type *type, char *varname) {
    if (type == NULL) {
        str_append_chars(append_to, "void");
        fc_write_c_type_varname(append_to, type, varname);
        return;
    }
    //
    if (type->type == type_bool) {
        str_append_chars(append_to, "unsigned char");
        fc_write_c_type_varname(append_to, type, varname);
        return;
    }
    //
    if (type->type == type_void_pointer) {
        str_append_chars(append_to, "void*");
        fc_write_c_type_varname(append_to, type, varname);
        return;
    }
    //
    if (type->type == type_funcref) {
        // {ret_type} (*{varname})({arg1_type}, {arg2_type}) = ...
        fc_write_c_type(append_to, type->func_return_type, NULL);
        str_append_chars(append_to, "(*");
        fc_write_c_type_varname(append_to, type, varname);
        str_append_chars(append_to, ")(");
        for (int i = 0; i < type->func_arg_types->length; i++) {
            Type *arg_type = array_get_index(type->func_arg_types, i);
            if (i > 0) {
                str_append_chars(append_to, ", ");
            }
            fc_write_c_type(append_to, arg_type, NULL);
        }
        str_append_chars(append_to, ")");
        return;
    }
    //
    if (type->type == type_struct) {
        Class *class = type->class;
        if (class->is_number) {
            int bytes = type->bytes;
            if (class->is_unsigned) {
                str_append_chars(append_to, "unsigned ");
            }
            while (bytes > 4) {
                bytes /= 2;
                str_append_chars(append_to, "long ");
            }
            if (bytes == 1) {
                str_append_chars(append_to, "char");
            } else if (bytes == 2) {
                str_append_chars(append_to, "short");
            } else if (bytes == 4) {
                str_append_chars(append_to, "int");
            } else {
                die("Cant determine c number type based on the amount of bytes");
            }
        } else {
            // Normal class
            if (!class->is_ctype) {
                str_append_chars(append_to, "struct ");
            }
            str_append_chars(append_to, class->cname);
        }

        if (type->is_pointer) {
            str_append_chars(append_to, "*");
        }
        fc_write_c_type_varname(append_to, type, varname);
        return;
    }
    if (type->type == type_enum) {
        str_append_chars(append_to, "int");
        fc_write_c_type_varname(append_to, type, varname);
        return;
    }
    if (type->type == type_null) {
        str_append_chars(append_to, "void");
        fc_write_c_type_varname(append_to, type, varname);
        return;
    }
    if (type->type == type_void) {
        str_append_chars(append_to, "void");
        fc_write_c_type_varname(append_to, type, varname);
        return;
    }
    printf("Could not convert type to c: %d\n", type->type);
    raise(SIGSEGV); // Useful for debugging
}

void fc_write_c_if(FileCompiler *fc, TokenIf *ift) {
    //
    if (ift->is_else) {
        str_append_chars(fc->tkn_buffer, " else {\n");
    }
    if (ift->condition) {
        fc_write_c_value(fc, ift->condition, true);
        str_append_chars(fc->tkn_buffer, "if (");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ") {\n");
    }

    fc_write_c_ast(fc, ift->scope);

    if (ift->condition) {
        str_append_chars(fc->tkn_buffer, "}\n");
    }

    if (ift->next) {
        fc_write_c_if(fc, ift->next);
    }

    if (ift->is_else) {
        str_append_chars(fc->tkn_buffer, "}\n");
    }
}

Str *value_buf(FileCompiler *fc) {
    fc->value_buffer->length = 0;
    return fc->value_buffer;
}

char *var_buf(FileCompiler *fc) {
    strcpy(fc->var_buf, "_KI_VBUF");
    fc->var_bufc++;
    sprintf(fc->sprintf, "%d", fc->var_bufc);
    strcat(fc->var_buf, fc->sprintf);
    return fc->var_buf;
}

void deref_local_vars(FileCompiler *fc, Value *retv, Scope *until_scope) {
    //
    char *ignore_vbuf = NULL;
    if (retv && retv->return_type->class && retv->return_type->class->ref_count) {
        ignore_vbuf = str_to_chars(fc->value_buffer);
    }
    //
    Scope *scope = fc->current_scope;

    // If we want to assign new values to argument variables, this code is required
    // Currently we dont allow this, because of performance (extra ref counting)
    // Scope *func_scope = scope;
    // while (func_scope && !func_scope->func) {
    //     func_scope = func_scope->parent;
    // }

    // Write + Clear var bufs
    int c = 0;
    while (true) {
        c++;

        for (int i = 0; i < scope->var_bufs->length; i++) {
            VarInfo *vi = array_get_index(scope->var_bufs, i);
            char *vb = vi->name;
            Type *rt = vi->return_type;

            str_append_chars(fc->tkn_buffer, "if(");
            if (rt->nullable) {
                str_append_chars(fc->tkn_buffer, vb);
                str_append_chars(fc->tkn_buffer, " && ");
            }
            str_append_chars(fc->tkn_buffer, "--");
            str_append_chars(fc->tkn_buffer, vb);
            str_append_chars(fc->tkn_buffer, "->_RC == 0) ");
            if (ignore_vbuf && strcmp(ignore_vbuf, vb) == 0) {
                str_append_chars(fc->tkn_buffer, "{}\n");
            } else {
                str_append_chars(fc->tkn_buffer, rt->class->cname);
                str_append_chars(fc->tkn_buffer, "____free(");
                str_append_chars(fc->tkn_buffer, vb);
                str_append_chars(fc->tkn_buffer, ");\n");
            }

            if (c == 1) {
                free(vb);
                free(vi);
            }
        }

        if (c == 1) {
            scope->var_bufs->length = 0;
        }

        // Clear local vars
        Array *local_vars = scope->local_var_names;
        for (int i = 0; i < local_vars->length; i++) {
            TokenDeclare *decl = array_get_index(local_vars, i);
            Class *class = decl->type->class;
            bool nullable = decl->type->nullable;
            char *lv = decl->name;

            if (nullable) {
                str_append_chars(fc->tkn_buffer, "if(");
                str_append_chars(fc->tkn_buffer, lv);
                str_append_chars(fc->tkn_buffer, "){ ");
            }
            str_append_chars(fc->tkn_buffer, "if(--");
            str_append_chars(fc->tkn_buffer, lv);
            str_append_chars(fc->tkn_buffer, "->_RC == 0) ");

            if (ignore_vbuf && strcmp(ignore_vbuf, lv) == 0) {
                str_append_chars(fc->tkn_buffer, "{}");
            } else {
                str_append_chars(fc->tkn_buffer, class->cname);
                str_append_chars(fc->tkn_buffer, "____free(");
                str_append_chars(fc->tkn_buffer, lv);
                str_append_chars(fc->tkn_buffer, ");");
            }

            if (nullable) {
                str_append_chars(fc->tkn_buffer, " }");
            }
            str_append_chars(fc->tkn_buffer, "\n");
        }

        if (ignore_vbuf) {
            free(ignore_vbuf);
            ignore_vbuf = NULL;
        }

        if (scope->is_func) {
            break;
        }
        if (until_scope && scope == until_scope) {
            break;
        }

        scope = scope->parent;

        if (scope == NULL) {
            break;
        }
    }

    // If we want to assign new values to argument variables, this code is required
    // Currently we dont allow this, because of performance (extra ref counting)
    // // Deref func args
    // if (!once || scope->func) {
    //     if (func_scope) {
    //         Function *func = func_scope->func;
    //         int arg_len = func->args->length;
    //         int x = 0;
    //         while (x < arg_len) {
    //             FunctionArg *arg = array_get_index(func->args, x);
    //             char *name = arg->name;
    //             Type *type = arg->type;
    //             x++;
    //             Class *class = type->class;
    //             if (class && class->ref_count) {
    //                 if (type->nullable) {
    //                     str_append_chars(fc->tkn_buffer, "if(");
    //                     str_append_chars(fc->tkn_buffer, name);
    //                     str_append_chars(fc->tkn_buffer, ") ");
    //                 }
    //                 str_append_chars(fc->tkn_buffer, name);
    //                 str_append_chars(fc->tkn_buffer, "->_RC--;\n");
    //             }
    //         }
    //     }
    // }
}

char *fc_write_c_get_allocator(FileCompiler *fc, int size, bool threaded) {
    size += 24;
    threaded = false; // force unthreaded
    //
    sprintf(fc->sprintf, "KI_allocator_%d_%d", size, threaded);
    char *name = fc->sprintf;

    char *last = map_get(allocators, name);
    if (last) {
        // printf("Already has %s\n", name);
        return last;
    }
    // printf("Create %s\n", name);
    // printf("-> %s\n", fc->c_filepath);

    name = strdup(name);
    sprintf(fc->sprintf, "%d", size);

    str_append_chars(fc->h_code, threaded ? "struct pthread_key_t " : "void* ");
    str_append_chars(fc->h_code, name);
    str_append_chars(fc->h_code, "__TK;\n");

    str_append_chars(fc->c_code_after, threaded ? "struct pthread_key_t " : "void* ");
    str_append_chars(fc->c_code_after, name);
    str_append_chars(fc->c_code_after, "__TK;\n");

    str_append_chars(fc->h_code, "struct ki__mem__Allocator* ");
    str_append_chars(fc->h_code, name);
    str_append_chars(fc->h_code, "();\n");

    str_append_chars(fc->c_code_after, "struct ki__mem__Allocator* ");
    str_append_chars(fc->c_code_after, name);
    str_append_chars(fc->c_code_after, "(){\n");

    str_append_chars(fc->c_code_after, "struct ki__mem__Allocator* a = ");
    str_append_chars(fc->c_code_after, threaded ? "pthread_getspecific(" : "(");
    str_append_chars(fc->c_code_after, name);
    str_append_chars(fc->c_code_after, "__TK);\n");

    str_append_chars(fc->c_code_after, "if(a){ return a; }\n");

    str_append_chars(fc->c_code_after, "a = ki__mem__Allocator__make(");
    str_append_chars(fc->c_code_after, fc->sprintf);
    str_append_chars(fc->c_code_after, ");\n");

    if (threaded) {
        str_append_chars(fc->c_code_after, "pthread_setspecific(");
        str_append_chars(fc->c_code_after, name);
        str_append_chars(fc->c_code_after, "__TK, a);\n");
    } else {
        str_append_chars(fc->c_code_after, name);
        str_append_chars(fc->c_code_after, "__TK = a;\n");
    }

    str_append_chars(fc->c_code_after, "return a;\n");
    str_append_chars(fc->c_code_after, "}\n");

    map_set(allocators, name, name);
    return name;
}
