
#include "../all.h"

void fc_write_c_all() {
  // Clear header
  char* path = malloc(KI_PATH_MAX);
  char* cache_dir = get_cache_dir();
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
    PkgCompiler* pkc = array_get_index(packages->values, i);
    for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
      FileCompiler* fc = array_get_index(pkc->file_compilers->values, o);

      for (int x = 0; x < fc->classes->length; x++) {
        Class* class = array_get_index(fc->classes, x);
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
    PkgCompiler* pkc = array_get_index(packages->values, i);
    for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
      FileCompiler* fc = array_get_index(pkc->file_compilers->values, o);

      for (int x = 0; x < fc->classes->length; x++) {
        Class* class = array_get_index(fc->classes, x);
        if (class->generic_names != NULL && class->generic_hash == NULL) {
          continue;
        }
        fc_write_c_class(fc, class);
      }
      for (int x = 0; x < fc->enums->length; x++) {
        fc_write_c_enum(fc, array_get_index(fc->enums, x));
      }

      for (int x = 0; x < fc->static_vars->length; x++) {
        fc_write_c_static_var_global(fc, array_get_index(fc->static_vars, x));
      }

      for (int x = 0; x < fc->threaded_globals->length; x++) {
        fc_write_c_threaded_globals(fc,
                                    array_get_index(fc->threaded_globals, x));
      }

      for (int x = 0; x < fc->mutexes->length; x++) {
        fc_write_c_mutex(fc, array_get_index(fc->mutexes, x));
      }

      fc_write_c_ast(fc, fc->scope);
      fc_write_c(fc);
    }
  }

  fc_write_c_inits();

  if (uses_async) {
    write_file(
        path,
        "void ki__async__Taskman__add_task(struct ki__async__Task* task);\n",
        true);
    write_file(path, "void ki__async__Taskman__suspend_task();\n", true);
    write_file(path, "void ki__async__Taskman__run_another_task();\n", true);
  }
  write_file(path, "void KI_INITS();\n", true);
}

void fc_write_c_pre(FileCompiler* fc) {
  char* structs_code = str_to_chars(fc->struct_code);

  char* path = malloc(KI_PATH_MAX);
  char* cache_dir = get_cache_dir();
  strcpy(path, cache_dir);
  strcat(path, "/project.h");
  write_file(path, structs_code, true);

  free(structs_code);
}

void fc_write_c_inits() {
  Str* code = str_make("");
  //
  char* hpath = malloc(KI_PATH_MAX);
  char* cache_dir = get_cache_dir();
  strcpy(hpath, cache_dir);
  strcat(hpath, "/project.h");
  //
  str_append_chars(code, "#include \"project.h\"\n\n");

  FileCompiler* inits_fc = init_fc();

  for (int i = 0; i < packages->keys->length; i++) {
    PkgCompiler* pkc = array_get_index(packages->values, i);
    for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
      FileCompiler* fc = array_get_index(pkc->file_compilers->values, o);

      for (int x = 0; x < fc->static_vars->length; x++) {
        TokenStaticDeclare* decl = array_get_index(fc->static_vars, x);

        fc_write_c_type(code, decl->scope->return_type, NULL);
        str_append_chars(code, " ");
        str_append_chars(code, decl->global_name);
        str_append_chars(code, "_init(){\n");

        inits_fc->tkn_buffer = str_make("");
        fc_write_c_ast(inits_fc, decl->scope);
        str_append(code, inits_fc->tkn_buffer);
        free_str(inits_fc->tkn_buffer);

        str_append_chars(code, "}\n");
      }
    }
  }

  str_append(code, inits_fc->c_code_after);
  write_file(hpath, str_to_chars(inits_fc->h_code), true);

  str_append_chars(code, "void KI_INITS(){\n");

  for (int i = 0; i < allocators->keys->length; i++) {
    char* name = array_get_index(allocators->values, i);
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
    PkgCompiler* pkc = array_get_index(packages->values, i);
    for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
      FileCompiler* fc = array_get_index(pkc->file_compilers->values, o);

      for (int x = 0; x < fc->static_vars->length; x++) {
        TokenStaticDeclare* decl = array_get_index(fc->static_vars, x);

        str_append_chars(code, decl->global_name);
        str_append_chars(code, " = ");
        str_append_chars(code, decl->global_name);
        str_append_chars(code, "_init();\n");

        Class* class = decl->type->class;
        bool nullable = decl->type->nullable;
        if (class && class->ref_count) {
          str_append_chars(code, decl->global_name);
          str_append_chars(code, "->_RC++;\n");
        }
      }

      for (int x = 0; x < fc->threaded_globals->length; x++) {
        ThreadedGlobal* tg = array_get_index(fc->threaded_globals, x);

        fc->tkn_buffer = str_make("");
        fc_write_c_value(fc, tg->default_value, true);
        str_append(code, fc->tkn_buffer);
        free_str(fc->tkn_buffer);

        str_append_chars(code, "pthread_key_create(&");
        str_append_chars(code, fc->nsc->pkc->name);
        str_append_chars(code, "__");
        str_append_chars(code, fc->nsc->name);
        str_append_chars(code, "__");
        str_append_chars(code, tg->name);
        str_append_chars(code, ", ");
        str_append(code, fc->value_buffer);
        str_append_chars(code, ");\n");
      }
    }
  }

  str_append_chars(code, "}\n");

  //
  char* code_ = str_to_chars(code);
  char* path = malloc(KI_PATH_MAX);
  strcpy(path, cache_dir);
  strcat(path, "/inits.c");
  write_file(path, code_, false);

  free(code_);
  free_str(code);
}

void fc_write_c(FileCompiler* fc) {
  // Write c + o file
  char* hcode = str_to_chars(fc->h_code);
  char* code = str_to_chars(fc->c_code);
  char* code_gen = str_to_chars(fc->c_code_after);

  // printf("code:\n");
  // printf("%s\n", code);

  fc->create_o_file = false;
  if (strlen(code) > 0 || strlen(code_gen) > 0) {
    fc->create_o_file = true;
    if (true) {
      write_file(fc->c_filepath, "\n#include \"project.h\"\n\n", false);

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
    char* path = malloc(KI_PATH_MAX);
    char* cache_dir = get_cache_dir();
    strcpy(path, cache_dir);
    strcat(path, "/project.h");
    write_file(path, hcode, true);
  }

  free(hcode);
  free(code);
  free(code_gen);
}

void fc_write_c_predefine_class(FileCompiler* fc, Class* class) {
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

void fc_write_c_class(FileCompiler* fc, Class* class) {
  //
  str_append_chars(fc->h_code, "struct ");
  str_append_chars(fc->h_code, class->cname);
  str_append_chars(fc->h_code, " {\n");
  for (int i = 0; i < class->props->keys->length; i++) {
    char* name = array_get_index(class->props->keys, i);
    ClassProp* prop = array_get_index(class->props->values, i);
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
  ClassProp* prop = map_get(class->props, "__free");
  if (!prop) {
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
      char* name = array_get_index(class->props->keys, i);
      ClassProp* prop = array_get_index(class->props->values, i);
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

    char* alloc_func = fc_write_c_get_allocator(fc, class->size, true);

    str_append_chars(fc->c_code, "ki__mem__Allocator__free(");
    str_append_chars(fc->c_code, alloc_func);
    str_append_chars(fc->c_code, "(), this);\n");
    str_append_chars(fc->c_code, "}\n\n");
  }
}

void fc_write_c_enum(FileCompiler* fc, Enum* enu) {
  //
  str_append_chars(fc->h_code, "typedef enum ");
  str_append_chars(fc->h_code, enu->cname);
  str_append_chars(fc->h_code, "{\n");

  int len = enu->values->keys->length;
  for (int i = 0; i < len; i++) {
    char* name = array_get_index(enu->values->keys, i);
    char* value = array_get_index(enu->values->values, i);
    str_append_chars(fc->h_code, name);
    str_append_chars(fc->h_code, " = ");
    str_append_chars(fc->h_code, value);
    str_append_chars(fc->h_code, ",\n");
  }

  str_append_chars(fc->h_code, "} ");
  str_append_chars(fc->h_code, enu->cname);
  str_append_chars(fc->h_code, ";\n");
}

void fc_write_c_static_var_global(FileCompiler* fc, TokenStaticDeclare* decl) {
  fc_write_c_type(fc->h_code, decl->scope->return_type, NULL);
  str_append_chars(fc->h_code, " ");
  str_append_chars(fc->h_code, decl->global_name);
  str_append_chars(fc->h_code, "_init();\n");

  fc_write_c_type(fc->h_code, decl->type, decl->global_name);
  str_append_chars(fc->h_code, ";\n");
}

void fc_write_c_threaded_globals(FileCompiler* fc, ThreadedGlobal* tg) {
  str_append_chars(fc->h_code, "struct pthread_key_t ");
  str_append_chars(fc->h_code, fc->nsc->pkc->name);
  str_append_chars(fc->h_code, "__");
  str_append_chars(fc->h_code, fc->nsc->name);
  str_append_chars(fc->h_code, "__");
  str_append_chars(fc->h_code, tg->name);
  str_append_chars(fc->h_code, ";\n");
}

void fc_write_c_mutex(FileCompiler* fc, Mutex* mut) {
  str_append_chars(fc->h_code, "struct pthread_mutex_t ");
  str_append_chars(fc->h_code, mut->cname);
  str_append_chars(fc->h_code, ";\n");
}

void fc_write_c_func(FileCompiler* fc, Function* func) {
  // Clear local var names array
  fc->local_var_names->length = 0;

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
    FunctionArg* arg = array_get_index(func->args, x);
    char* name = arg->name;
    Type* type = arg->type;
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
    str_append_chars(fc->tkn_buffer, "char* _KI_THROW_MSG");
    str_append_chars(fc->h_code, "char* _KI_THROW_MSG");
  }
  //
  str_append_chars(fc->h_code, ");\n");

  if (!fc->is_header) {
    str_append_chars(fc->tkn_buffer, ") {\n");
    if (func->scope->catch_errors) {
      str_append_chars(fc->tkn_buffer, "char* _KI_THROW_MSG_BUF = 0;\n");
    }
    fc->indent++;

    if (strcmp(func->cname, "main") == 0) {
      str_append_chars(fc->tkn_buffer, "KI_INITS();\n");
    }
    if (uses_async && strcmp(func->cname, "main") == 0) {
      str_append_chars(
          fc->tkn_buffer,
          "void* KI_MAIN_TMS = ki__async__Taskman__setup_task_managers();\n");
    }

    // Body
    fc_write_c_ast(fc, func->scope);

    if (uses_async && strcmp(func->cname, "main") == 0) {
      str_append_chars(
          fc->tkn_buffer,
          "ki__async__Taskman__wait_for_tasks_to_end(KI_MAIN_TMS);\n");
    }

    fc->indent--;
    str_append_chars(fc->tkn_buffer, "}\n\n");
  }
}

void fc_write_c_ast(FileCompiler* fc, Scope* scope) {
  Array* prev_local_vars = fc->local_var_names;
  Array* prev_var_bufs = fc->var_bufs;

  fc->local_var_names = array_make(8);
  fc->var_bufs = array_make(8);

  int c = 0;
  Array* ast = scope->ast;
  while (c < ast->length) {
    Token* t = array_get_index(ast, c);
    fc_write_c_token(fc, t);
    c++;
  }

  if (!scope->did_return) {
    deref_local_vars(fc);
  }

  free(fc->local_var_names);
  free(fc->var_bufs);
  fc->local_var_names = prev_local_vars;
  fc->var_bufs = prev_var_bufs;
}

void fc_write_c_token(FileCompiler* fc, Token* token) {
  Str* prev_buf = fc->tkn_buffer;
  Str* prev_before_buf = fc->before_tkn_buffer;
  Str* buf = str_make("");
  Str* before_buf = str_make("");
  fc->tkn_buffer = buf;
  fc->before_tkn_buffer = before_buf;
  //
  // printf("tt:%d\n", token->type);
  fc_indent(fc, fc->tkn_buffer);
  if (token->type == tkn_func) {
    fc_write_c_func(fc, token->item);
  } else if (token->type == tkn_static) {
    TokenStaticDeclare* decl = token->item;

    fc_write_c_type(fc->tkn_buffer, decl->type, decl->name);
    str_append_chars(fc->tkn_buffer, " = ");
    str_append_chars(fc->tkn_buffer, decl->global_name);
    str_append_chars(fc->tkn_buffer, ";\n");

  } else if (token->type == tkn_declare) {
    TokenDeclare* decl = token->item;

    fc_write_c_value(fc, decl->value, true);

    fc_write_c_type(fc->tkn_buffer, decl->type, decl->name);
    str_append_chars(fc->tkn_buffer, " = ");
    str_append(fc->tkn_buffer, fc->value_buffer);
    str_append_chars(fc->tkn_buffer, ";\n");

    Class* class = decl->value->return_type->class;
    bool nullable = decl->value->return_type->nullable;
    if (class && class->ref_count) {
      if (nullable) {
        str_append_chars(fc->tkn_buffer, "if(");
        str_append_chars(fc->tkn_buffer, decl->name);
        str_append_chars(fc->tkn_buffer, ") ");
      }
      str_append_chars(fc->tkn_buffer, decl->name);
      str_append_chars(fc->tkn_buffer, "->_RC++;\n");

      array_push(fc->local_var_names, decl);
    }
  } else if (token->type == tkn_assign) {
    TokenAssign* ta = token->item;

    fc_write_c_value(fc, ta->left, true);
    char* left = str_to_chars(fc->value_buffer);
    fc_write_c_value(fc, ta->right, true);

    bool refc = false;
    bool refc_nullable = false;
    Class* class = NULL;
    if (ta->type == op_eq) {
      Value* left = ta->left;
      class = left->return_type->class;
      if (class && class->ref_count) {
        refc = true;
        if (left->return_type->nullable) {
          refc_nullable = true;
        }
      }
    }

    // RC++ the new value first
    if (refc) {
      if (refc_nullable) {
        str_append_chars(fc->tkn_buffer, "if(");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, "){ ");
      }
      str_append(fc->tkn_buffer, fc->value_buffer);
      str_append_chars(fc->tkn_buffer, "->_RC++;");
      if (refc_nullable) {
        str_append_chars(fc->tkn_buffer, " }");
      }
      str_append_chars(fc->tkn_buffer, "\n");
    }

    // RC-- the old value
    if (refc) {
      str_append_chars(fc->tkn_buffer, "if(");
      str_append_chars(fc->tkn_buffer, left);
      str_append_chars(fc->tkn_buffer, "){ ");
      str_append_chars(fc->tkn_buffer, left);
      str_append_chars(fc->tkn_buffer, "->_RC--;\n");
      str_append_chars(fc->tkn_buffer, "if(");
      str_append_chars(fc->tkn_buffer, left);
      str_append_chars(fc->tkn_buffer, "->_RC == 0) ");
      str_append_chars(fc->tkn_buffer, class->cname);
      str_append_chars(fc->tkn_buffer, "____free(");
      str_append_chars(fc->tkn_buffer, left);
      str_append_chars(fc->tkn_buffer, ");");
      str_append_chars(fc->tkn_buffer, " }");
      str_append_chars(fc->tkn_buffer, "\n");
    }

    str_append_chars(fc->tkn_buffer, left);
    free(left);
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
  } else if (token->type == tkn_return) {
    Value* retv = NULL;
    if (token->item) {
      retv = token->item;
      fc_write_c_value(fc, token->item, true);
    }

    if (fc->local_var_names->length > 0 || fc->var_bufs->length > 0) {
      bool refc = false;
      if (retv && retv->return_type->class &&
          retv->return_type->class->ref_count) {
        refc = true;
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, "->_RC++;\n");
      }

      // Deref local vars + Check if var_bufs RC == 0 (if so free)
      deref_local_vars(fc);

      if (refc) {
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, "->_RC--;\n");
      }
    }

    //
    str_append_chars(fc->tkn_buffer, "return ");
    if (retv) {
      str_append(fc->tkn_buffer, fc->value_buffer);
    }
    str_append_chars(fc->tkn_buffer, ";\n");
  } else if (token->type == tkn_if) {
    fc_write_c_if(fc, token->item);
  } else if (token->type == tkn_while) {
    //
    TokenWhile* wt = token->item;
    fc_write_c_value(fc, wt->condition, true);
    str_append_chars(fc->tkn_buffer, "while(");
    str_append(fc->tkn_buffer, fc->value_buffer);
    str_append_chars(fc->tkn_buffer, ") {\n");
    fc_write_c_ast(fc, wt->scope);
    str_append_chars(fc->tkn_buffer, "}\n\n");
  } else if (token->type == tkn_break) {
    str_append_chars(fc->tkn_buffer, "break;\n");
  } else if (token->type == tkn_continue) {
    str_append_chars(fc->tkn_buffer, "continue;\n");
  } else if (token->type == tkn_throw) {
    TokenThrow* tt = token->item;
    str_append_chars(fc->tkn_buffer, "_KI_THROW_MSG = \"");
    str_append_chars(fc->tkn_buffer, tt->msg);
    str_append_chars(fc->tkn_buffer, "\";\n");
    if (tt->return_type == NULL) {
      str_append_chars(fc->tkn_buffer, "return;\n");
    } else if (tt->return_type->is_pointer) {
      str_append_chars(fc->tkn_buffer, "return 0;\n");
    } else {
      str_append_chars(fc->tkn_buffer, "return 0;\n");
    }
  } else if (token->type == tkn_task_suspend) {
    str_append_chars(fc->tkn_buffer,
                     "ki__async__Taskman__suspend_task(); return;\n");
  } else if (token->type == tkn_set_threaded) {
    TokenIdValue* iv = token->item;

    fc_write_c_value(fc, iv->value, true);

    str_append_chars(fc->tkn_buffer, "pthread_setspecific(");
    str_append_chars(fc->tkn_buffer, iv->name);
    str_append_chars(fc->tkn_buffer, ",");
    str_append(fc->tkn_buffer, fc->value_buffer);
    str_append_chars(fc->tkn_buffer, ");\n");

  } else if (token->type == tkn_mutex_init) {
    Value* val = token->item;

    fc_write_c_value(fc, val, true);

    // str_append_chars(fc->tkn_buffer, "pthread_mutexattr_t ma;\n");
    // str_append_chars(fc->tkn_buffer, "pthread_mutexattr_init(&ma);\n");
    // str_append_chars(
    //     fc->tkn_buffer,
    //     "pthread_mutexattr_setpshared(&ma, PTHREAD_PROCESS_SHARED);\n");
    // str_append_chars(
    //     fc->tkn_buffer,
    //     "pthread_mutexattr_setpshared(&ma, PTHREAD_MUTEX_ROBUST);\n");

    str_append_chars(fc->tkn_buffer, "pthread_mutex_init(");
    str_append(fc->tkn_buffer, fc->value_buffer);
    str_append_chars(fc->tkn_buffer, ", (void*)0);\n");

  } else if (token->type == tkn_mutex_lock) {
    Value* val = token->item;

    fc_write_c_value(fc, val, true);

    str_append_chars(fc->tkn_buffer, "pthread_mutex_lock(&");
    str_append(fc->tkn_buffer, fc->value_buffer);
    str_append_chars(fc->tkn_buffer, ");\n");

  } else if (token->type == tkn_mutex_unlock) {
    Value* val = token->item;

    fc_write_c_value(fc, val, true);

    str_append_chars(fc->tkn_buffer, "pthread_mutex_unlock(&");
    str_append(fc->tkn_buffer, fc->value_buffer);
    str_append_chars(fc->tkn_buffer, ");\n");

  } else if (token->type == tkn_free) {
    fc_write_c_value(fc, token->item, true);
    str_append_chars(fc->tkn_buffer, "ki__mem__free(");
    str_append(fc->tkn_buffer, fc->value_buffer);
    str_append_chars(fc->tkn_buffer, ");\n");
  } else if (token->type == tkn_value) {
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

char* indenter;
void fc_indent(FileCompiler* fc, Str* append_to) {
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

void fc_write_c_value(FileCompiler* fc, Value* value, bool new_value) {
  Str* result = fc->value_buffer;

  if (new_value) {
    result->length = 0;
  }

  //
  // printf("Type: %d\n", value->type);
  if (value->type == vt_string) {
    char* buf_var_name = strdup(var_buf(fc));

    str_append_chars(fc->tkn_buffer, "struct ki__type__string* ");
    str_append_chars(fc->tkn_buffer, buf_var_name);
    str_append_chars(fc->tkn_buffer, " = ki__type__string__make(\"");
    char* str = value->item;
    str_append_chars(fc->tkn_buffer, str);
    str_append_chars(fc->tkn_buffer, "\", ");
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
    str_append_chars(fc->tkn_buffer, lenstr);
    str_append_chars(fc->tkn_buffer, ");\n");

    str_append_chars(fc->tkn_buffer, buf_var_name);
    str_append_chars(fc->tkn_buffer, "->_RC++;\n");

    str_append_chars(result, buf_var_name);

    VarInfo* vi = malloc(sizeof(VarInfo));
    vi->name = buf_var_name;
    // todo: this leaks memory
    vi->return_type =
        fc_identifier_to_type(fc, create_identifier("ki", "type", "string"));
    //

    array_push(fc->var_bufs, vi);

  } else if (value->type == vt_null) {
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
  } else if (value->type == vt_mutex) {
    str_append_chars(result, value->item);
  } else if (value->type == vt_number) {
    str_append_chars(result, value->item);
  } else if (value->type == vt_char) {
    str_append_chars(result, "'");
    str_append_chars(result, value->item);
    str_append_chars(result, "'");
  } else if (value->type == vt_operator) {
    ValueOperator* op = value->item;
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
    ValueFuncCall* fa = value->item;

    // Arg values
    char* cache = str_to_chars(fc->value_buffer);
    Array* arg_strings = array_make(4);
    for (int i = 0; i < fa->arg_values->length; i++) {
      Value* v = array_get_index(fa->arg_values, i);
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
      char* arg_str = array_get_index(arg_strings, i);
      str_append_chars(result, arg_str);
      free(arg_str);
    }
    free(arg_strings);

    if (fa->on->return_type->func_can_error) {
      if (fa->arg_values->length > 0) {
        str_append_chars(result, ", ");
      }
      str_append_chars(result, "_KI_THROW_MSG_BUF");
    }
    str_append_chars(result, ")");

    if (value->return_type) {
      Class* retClass = value->return_type->class;
      if (retClass && retClass->ref_count) {
        // Buffer the value
        char* buf_var_name = strdup(var_buf(fc));
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

        VarInfo* vi = malloc(sizeof(VarInfo));
        vi->name = buf_var_name;
        vi->return_type = value->return_type;

        array_push(fc->var_bufs, vi);
      }
    }

  } else if (value->type == vt_sizeof) {
    str_append_chars(result, value->item);
  } else if (value->type == vt_cast) {
    ValueCast* cast = value->item;
    str_append_chars(result, "(");
    fc_write_c_type(result, cast->as_type, NULL);
    str_append_chars(result, ")");
    fc_write_c_value(fc, cast->value, false);
  } else if (value->type == vt_getptrv) {
    ValueCast* cast = value->item;
    str_append_chars(result, "*(");
    fc_write_c_type(result, cast->as_type, NULL);
    str_append_chars(result, "*)(");
    fc_write_c_value(fc, cast->value, false);
    str_append_chars(result, ")");
  } else if (value->type == vt_getptr) {
    str_append_chars(result, "&");
    fc_write_c_value(fc, value->item, false);
  } else if (value->type == vt_setptrv) {
    SetPtrValue* cast = value->item;
    str_append_chars(result, "*(");
    fc_write_c_type(result, cast->to_value->return_type, NULL);
    str_append_chars(result, "*)");
    fc_write_c_value(fc, cast->ptr_value, false);
    str_append_chars(result, " = ");
    fc_write_c_value(fc, cast->to_value, false);
  } else if (value->type == vt_class_init) {
    // Generate function
    GEN_C++;
    ValueClassInit* ini = value->item;
    Class* class = ini->class;

    char* allocator_name = fc_write_c_get_allocator(fc, class->size, true);
    char* func_name = malloc(30);
    sprintf(func_name, "_KI_CLASS_INIT_%d", GEN_C);

    char* buf_var_name = strdup(var_buf(fc));
    str_append_chars(result, buf_var_name);

    // Set cache
    char* cache = str_to_chars(fc->value_buffer);

    //
    Str* args_str = str_make("");
    for (int i = 0; i < ini->prop_values->values->length; i++) {
      if (i > 0) {
        str_append_chars(args_str, ", ");
      }
      Value* val = array_get_index(ini->prop_values->values, i);
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
      char* prop_name = array_get_index(ini->prop_values->keys, i);
      ClassProp* prop = map_get(class->props, prop_name);
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
      char* prop_name = array_get_index(ini->prop_values->keys, i);
      ClassProp* prop = map_get(class->props, prop_name);
      fc_write_c_type(fc->c_code_after, prop->return_type, prop_name);
    }
    str_append_chars(fc->c_code_after, ") {\n");
    char* sign;
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
      char* prop_name = array_get_index(ini->prop_values->keys, i);
      Value* v = array_get_index(ini->prop_values->values, i);
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

    Array* prev_local_vars = fc->local_var_names;
    Array* prev_var_bufs = fc->var_bufs;

    fc->local_var_names = array_make(8);
    fc->var_bufs = array_make(8);

    Str* prevbuf = fc->tkn_buffer;
    fc->tkn_buffer = str_make("");

    for (int i = 0; i < class->props->keys->length; i++) {
      char* prop_name = array_get_index(class->props->keys, i);
      if (map_contains(ini->prop_values, prop_name)) {
        continue;
      }
      ClassProp* prop = array_get_index(class->props->values, i);
      if (!prop->default_value) {
        continue;
      }

      char* cache = str_to_chars(fc->value_buffer);
      fc->value_buffer->length = 0;
      fc_write_c_value(fc, prop->default_value, false);
      char* defv = str_to_chars(fc->value_buffer);
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

    deref_local_vars(fc);

    str_append(fc->c_code_after, fc->tkn_buffer);
    free_str(fc->tkn_buffer);
    fc->tkn_buffer = prevbuf;

    free(fc->local_var_names);
    free(fc->var_bufs);
    fc->local_var_names = prev_local_vars;
    fc->var_bufs = prev_var_bufs;

    str_append_chars(fc->c_code_after, "return KI_RET_V;\n");
    str_append_chars(fc->c_code_after, "}\n\n");

  } else if (value->type == vt_prop_access) {
    ValueClassPropAccess* pa = value->item;

    if (pa->is_static) {
      Class* class = pa->on;
      // ClassProp* prop = map_get(class->props, pa->name);
      //  func ref
      //  Type* type = prop->return_type;
      str_append_chars(result, class->cname);
      str_append_chars(result, "__");
      str_append_chars(result, pa->name);
    } else {
      Value* val = pa->on;
      fc_write_c_value(fc, pa->on, false);
      Type* type = val->return_type;
      if (type->is_pointer) {
        str_append_chars(result, "->");
      } else {
        str_append_chars(result, ".");
      }
      str_append_chars(result, pa->name);
    }
  } else if (value->type == vt_async) {
    Value* fcallv = value->item;
    ValueFuncCall* fcall = fcallv->item;
    Value* on = fcall->on;
    char* size = malloc(10);
    //
    Type* task_type =
        fc_identifier_to_type(fc, create_identifier("ki", "async", "Task"));
    // Cache current value
    char* cache = str_to_chars(fc->value_buffer);

    // Step 1. Generate execution function
    char* handler_name = strdup(var_buf(fc));
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
    Array* arg_strings = array_make(4);
    for (int i = 0; i < fcall->arg_values->length; i++) {
      char* arg_name = malloc(10);
      Value* v = array_get_index(fcall->arg_values, i);
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
    char* func_ref_name = strdup(var_buf(fc));
    str_append_chars(fc->c_code_after, "void* ");
    str_append_chars(fc->c_code_after, func_ref_name);
    str_append_chars(fc->c_code_after, " = task->func;\n");

    char* ret_name = NULL;
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
      char* arg_name = array_get_index(arg_strings, i);
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
    str_append_chars(fc->c_code_after, "if(!task->suspended){\n");
    str_append_chars(fc->c_code_after, "task->ready = 1;\n");
    // Deref args if needed
    for (int i = 0; i < fcall->arg_values->length; i++) {
      Value* v = array_get_index(fcall->arg_values, i);
      char* arg_name = array_get_index(arg_strings, i);
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
    str_append_chars(fc->c_code_after, "}\n");
    // End body
    str_append_chars(fc->c_code_after, "}\n\n");

    // Step 2. Create Task and push onto stack
    // Func ref
    char* allocator_name =
        fc_write_c_get_allocator(fc, task_type->class->size, false);
    char* func_name = strdup(var_buf(fc));
    str_append_chars(fc->tkn_buffer, "void* ");
    str_append_chars(fc->tkn_buffer, func_name);
    str_append_chars(fc->tkn_buffer, " = ");
    fc->value_buffer->length = 0;
    fc_write_c_value(fc, on, false);
    str_append(fc->tkn_buffer, fc->value_buffer);
    str_append_chars(fc->tkn_buffer, ";\n");
    // Init Task
    char* var_name = strdup(var_buf(fc));
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
    str_append_chars(fc->tkn_buffer, "->jmpbuf = 0;\n");
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

    char* argsptr_name = var_buf(fc);
    str_append_chars(fc->tkn_buffer, "void* ");
    str_append_chars(fc->tkn_buffer, argsptr_name);
    str_append_chars(fc->tkn_buffer, " = ");
    str_append_chars(fc->tkn_buffer, var_name);
    str_append_chars(fc->tkn_buffer, "->args;\n");

    for (int i = 0; i < fcall->arg_values->length; i++) {
      Value* v = array_get_index(fcall->arg_values, i);
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

    // free (if Task has ref counting enabled)
    // VarInfo* vi = malloc(sizeof(VarInfo));
    // vi->name = var_name;
    // vi->return_type = value->return_type;

    // array_push(fc->var_bufs, vi);

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
    str_append_chars(fc->tkn_buffer,
                     "ki__async__Taskman__run_another_task();\n");
    str_append_chars(fc->tkn_buffer, "}\n");

    str_append_chars(result, "->result");
    //
  } else if (value->type == vt_allocator) {
    char* size = value->item;
    int sizei = atoi(size);
    char* name = fc_write_c_get_allocator(fc, sizei, true);
    str_append_chars(result, name);
    str_append_chars(result, "()");
  } else if (value->type == vt_get_threaded) {
    char* name = value->item;
    str_append_chars(result, "pthread_getspecific(");
    str_append_chars(result, name);
    str_append_chars(result, ")");
  } else {
    fc_error(fc, "Unhandled value token (compiler bug)", NULL);
  }
}

char i_to_str_buf[100];

void fc_write_c_type_varname(Str* append_to, Type* type, char* varname) {
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

void fc_write_c_type(Str* append_to, Type* type, char* varname) {
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
      Type* arg_type = array_get_index(type->func_arg_types, i);
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
    Class* class = type->class;
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
  raise(SIGSEGV);  // Useful for debugging
}

void fc_write_c_if(FileCompiler* fc, TokenIf* ift) {
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

Str* value_buf(FileCompiler* fc) {
  fc->value_buffer->length = 0;
  return fc->value_buffer;
}

char* var_buf(FileCompiler* fc) {
  strcpy(fc->var_buf, "_KI_VBUF");
  fc->var_bufc++;
  sprintf(fc->sprintf, "%d", fc->var_bufc);
  strcat(fc->var_buf, fc->sprintf);
  return fc->var_buf;
}

void deref_local_vars(FileCompiler* fc) {
  // Write + Clear var bufs
  for (int i = 0; i < fc->var_bufs->length; i++) {
    VarInfo* vi = array_get_index(fc->var_bufs, i);
    char* vb = vi->name;
    Type* rt = vi->return_type;

    str_append_chars(fc->tkn_buffer, "if(");
    if (rt->nullable) {
      str_append_chars(fc->tkn_buffer, vb);
      str_append_chars(fc->tkn_buffer, " && ");
    }
    str_append_chars(fc->tkn_buffer, "--");
    str_append_chars(fc->tkn_buffer, vb);
    str_append_chars(fc->tkn_buffer, "->_RC == 0) ");
    str_append_chars(fc->tkn_buffer, rt->class->cname);
    str_append_chars(fc->tkn_buffer, "____free(");
    str_append_chars(fc->tkn_buffer, vb);
    str_append_chars(fc->tkn_buffer, ");\n");

    free(vb);
    free(vi);
  }

  fc->var_bufs->length = 0;

  // Clear local vars
  Array* local_vars = fc->local_var_names;
  for (int i = 0; i < local_vars->length; i++) {
    TokenDeclare* decl = array_get_index(local_vars, i);
    Class* class = decl->type->class;
    bool nullable = decl->type->nullable;
    char* lv = decl->name;

    if (nullable) {
      str_append_chars(fc->tkn_buffer, "if(");
      str_append_chars(fc->tkn_buffer, lv);
      str_append_chars(fc->tkn_buffer, "){ ");
    }
    str_append_chars(fc->tkn_buffer, "if(--");
    str_append_chars(fc->tkn_buffer, lv);
    str_append_chars(fc->tkn_buffer, "->_RC == 0) ");
    str_append_chars(fc->tkn_buffer, class->cname);
    str_append_chars(fc->tkn_buffer, "____free(");
    str_append_chars(fc->tkn_buffer, lv);
    str_append_chars(fc->tkn_buffer, ");");
    if (nullable) {
      str_append_chars(fc->tkn_buffer, " }");
    }
    str_append_chars(fc->tkn_buffer, "\n");
  }
}

char* fc_write_c_get_allocator(FileCompiler* fc, int size, bool threaded) {
  size += 24;
  threaded = false;  // force unthreaded
  //
  sprintf(fc->sprintf, "KI_allocator_%d_%d", size, threaded);
  char* name = fc->sprintf;

  char* last = map_get(allocators, name);
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

  str_append_chars(fc->c_code_after,
                   threaded ? "struct pthread_key_t " : "void* ");
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

  str_append_chars(fc->c_code_after, "a = ki__mem__alloc_flat(64);\n");
  str_append_chars(fc->c_code_after, "a->size = ");
  str_append_chars(fc->c_code_after, fc->sprintf);
  str_append_chars(fc->c_code_after, ";\n");

  str_append_chars(fc->c_code_after, "a->block_i = 0;\n");
  str_append_chars(fc->c_code_after, "a->block_c = 0;\n");
  str_append_chars(fc->c_code_after,
                   "a->blocks_ptr = ki__mem__alloc_flat(256 * 8);\n");
  str_append_chars(fc->c_code_after, "a->mut = ki__async__Mutex__make();\n");

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
