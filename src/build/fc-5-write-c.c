
#include "../all.h"

void fc_write_c_all() {
  // Clear header
  char* path = malloc(KI_PATH_MAX);
  char* cache_dir = get_cache_dir();
  strcpy(path, cache_dir);
  strcat(path, "/project.h");
  write_file(path, "", false);
  write_file(path, "#include <stdbool.h>\n", true);
  write_file(path, "#include <stdlib.h>\n", true);
  // write_file(path, "#include <unistd.h>\n", true);

#ifdef __linux__
  // Todo delete this after .kh files can be converted to .h
  // write_file(path, "#include <sys/mman.h>\n", true);
#endif

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

  //
  for (int i = 0; i < packages->keys->length; i++) {
    PkgCompiler* pkc = array_get_index(packages->values, i);
    for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
      FileCompiler* fc = array_get_index(pkc->file_compilers->values, o);

      for (int x = 0; x < fc->classes->length; x++) {
        fc_write_c_class(fc, array_get_index(fc->classes, x));
      }
      for (int x = 0; x < fc->enums->length; x++) {
        fc_write_c_enum(fc, array_get_index(fc->enums, x));
      }

      fc_write_c_ast(fc, fc->scope->ast);
      fc_write_c(fc);
    }
  }
}

void fc_write_c(FileCompiler* fc) {
  // Write c + o file
  char* hcode = str_to_chars(fc->h_code);
  char* code = str_to_chars(fc->c_code);
  char* code_gen = str_to_chars(fc->c_code_after);

  // printf("code:\n");
  // printf("%s\n", code);

  fc->create_o_file = false;
  if (!fc->is_header && strlen(code) > 0) {
    fc->create_o_file = true;
    write_file(fc->c_filepath, "\n#include \"project.h\"\n\n", false);

    char* incl = malloc(KI_PATH_MAX + 50);
    for (int i = 0; i < fc->include_headers_from->length; i++) {
      FileCompiler* hfc = array_get_index(fc->include_headers_from, i);
      strcpy(incl, "#include \"");
      strcat(incl, hfc->h_filepath);
      strcat(incl, "\"\n");
      write_file(fc->c_filepath, incl, true);
    }
    free(incl);

    write_file(fc->c_filepath, "\n", true);
    write_file(fc->c_filepath, code, true);
    write_file(fc->c_filepath, code_gen, true);

    array_push(o_files, fc->o_filepath);
  }

  write_file(fc->h_filepath, hcode, false);

  char* path = malloc(KI_PATH_MAX);
  char* cache_dir = get_cache_dir();
  strcpy(path, cache_dir);
  strcat(path, "/project.h");
  write_file(path, hcode, true);

  free(hcode);
  free(code);
  free(code_gen);
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
    fc_write_c_type(fc->h_code, prop->return_type);
    str_append_chars(fc->h_code, " ");
    str_append_chars(fc->h_code, name);
    str_append_chars(fc->h_code, ";\n");
  }
  str_append_chars(fc->h_code, "};\n");
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
    str_append_chars(fc->h_code, " ");
    str_append_chars(fc->h_code, value);
    str_append_chars(fc->h_code, ",\n");
  }

  str_append_chars(fc->h_code, "} ");
  str_append_chars(fc->h_code, enu->cname);
  str_append_chars(fc->h_code, ";");
}

void fc_write_c_func(FileCompiler* fc, Function* func) {
  //
  fc_write_c_type(fc->tkn_buffer, func->return_type);
  fc_write_c_type(fc->h_code, func->return_type);
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
    fc_write_c_type(fc->tkn_buffer, type);
    fc_write_c_type(fc->h_code, type);
    str_append_chars(fc->tkn_buffer, " ");
    str_append_chars(fc->tkn_buffer, name);
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
      str_append_chars(fc->tkn_buffer, "char* _KI_THROW_MSG_BUF = NULL;\n");
    }
    fc->indent++;
    fc_write_c_ast(fc, func->scope->ast);
    fc->indent--;
    str_append_chars(fc->tkn_buffer, "}\n\n");
  }
}

void fc_write_c_ast(FileCompiler* fc, Array* ast) {
  int c = 0;
  while (c < ast->length) {
    Token* t = array_get_index(ast, c);
    fc_write_c_token(fc, t);
    c++;
  }
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
  } else if (token->type == tkn_declare) {
    TokenDeclare* decl = token->item;
    fc_write_c_type(fc->tkn_buffer, decl->value->return_type);
    str_append_chars(fc->tkn_buffer, " ");
    str_append_chars(fc->tkn_buffer, decl->name);
    str_append_chars(fc->tkn_buffer, " = ");
    fc_write_c_value(fc, decl->value);
    str_append_chars(fc->tkn_buffer, ";\n");
  } else if (token->type == tkn_assign) {
    TokenAssign* ta = token->item;
    fc_write_c_value(fc, ta->left);
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
    } else {
      fc_error(fc, "Unhandled assign operator translation", NULL);
    }
    fc_write_c_value(fc, ta->right);
    str_append_chars(fc->tkn_buffer, ";\n");
  } else if (token->type == tkn_return) {
    str_append_chars(fc->tkn_buffer, "return ");
    fc_write_c_value(fc, token->item);
    str_append_chars(fc->tkn_buffer, ";\n");
  } else if (token->type == tkn_if) {
    fc_write_c_if(fc, token->item);
  } else if (token->type == tkn_while) {
    //
    TokenWhile* wt = token->item;
    str_append_chars(fc->tkn_buffer, "while(");
    fc_write_c_value(fc, wt->condition);
    str_append_chars(fc->tkn_buffer, ") {\n");
    fc_write_c_ast(fc, wt->scope->ast);
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
      str_append_chars(fc->tkn_buffer, "return NULL;\n");
    } else {
      str_append_chars(fc->tkn_buffer, "return 0;\n");
    }
  } else if (token->type == tkn_free) {
    str_append_chars(fc->tkn_buffer, "free(");
    fc_write_c_value(fc, token->item);
    str_append_chars(fc->tkn_buffer, ");\n");
  } else if (token->type == tkn_value) {
    fc_write_c_value(fc, token->item);
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

void fc_write_c_value(FileCompiler* fc, Value* value) {
  //
  // printf("Type: %d\n", value->type);
  if (value->type == vt_string) {
    str_append_chars(fc->tkn_buffer, "ki__type__string__make(\"");
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
    str_append_chars(fc->tkn_buffer, ")");
  } else if (value->type == vt_null) {
    str_append_chars(fc->tkn_buffer, "NULL");
    // Bools
  } else if (value->type == vt_false) {
    str_append_chars(fc->tkn_buffer, "false");
  } else if (value->type == vt_true) {
    str_append_chars(fc->tkn_buffer, "true");
  } else if (value->type == vt_group) {
    str_append_chars(fc->tkn_buffer, "(");
    fc_write_c_value(fc, value->item);
    str_append_chars(fc->tkn_buffer, ")");
  } else if (value->type == vt_var) {
    str_append_chars(fc->tkn_buffer, value->item);
  } else if (value->type == vt_number) {
    str_append_chars(fc->tkn_buffer, value->item);
  } else if (value->type == vt_char) {
    str_append_chars(fc->tkn_buffer, "'");
    str_append_chars(fc->tkn_buffer, value->item);
    str_append_chars(fc->tkn_buffer, "'");
  } else if (value->type == vt_operator) {
    ValueOperator* op = value->item;
    fc_write_c_value(fc, op->left);
    if (op->type == op_add) {
      str_append_chars(fc->tkn_buffer, " + ");
    } else if (op->type == op_sub) {
      str_append_chars(fc->tkn_buffer, " - ");
    } else if (op->type == op_mult) {
      str_append_chars(fc->tkn_buffer, " * ");
    } else if (op->type == op_div) {
      str_append_chars(fc->tkn_buffer, " / ");
    } else if (op->type == op_mod) {
      str_append_chars(fc->tkn_buffer, " \% ");
      //
    } else if (op->type == op_eq) {
      str_append_chars(fc->tkn_buffer, " == ");
    } else if (op->type == op_neq) {
      str_append_chars(fc->tkn_buffer, " != ");
    } else if (op->type == op_lt) {
      str_append_chars(fc->tkn_buffer, " < ");
    } else if (op->type == op_lte) {
      str_append_chars(fc->tkn_buffer, " <= ");
    } else if (op->type == op_gt) {
      str_append_chars(fc->tkn_buffer, " > ");
    } else if (op->type == op_gte) {
      str_append_chars(fc->tkn_buffer, " >= ");
    } else if (op->type == op_incr) {
      str_append_chars(fc->tkn_buffer, "++");
    } else if (op->type == op_decr) {
      str_append_chars(fc->tkn_buffer, "--");
    } else {
      printf("Op: %d\n", op->type);
      fc_error(fc, "Unhandled operator type", NULL);
    }
    if (op->right) {
      fc_write_c_value(fc, op->right);
    }
  } else if (value->type == vt_func_call) {
    ValueFuncCall* fa = value->item;
    Function* func = fa->on->return_type->func;
    fc_write_c_value(fc, fa->on);
    str_append_chars(fc->tkn_buffer, "(");
    for (int i = 0; i < fa->arg_values->length; i++) {
      if (i > 0) {
        str_append_chars(fc->tkn_buffer, ", ");
      }
      Value* v = array_get_index(fa->arg_values, i);
      fc_write_c_value(fc, v);
    }
    if (func->can_error) {
      if (fa->arg_values->length > 0) {
        str_append_chars(fc->tkn_buffer, ", ");
      }
      str_append_chars(fc->tkn_buffer, "_KI_THROW_MSG_BUF");
    }
    str_append_chars(fc->tkn_buffer, ")");
  } else if (value->type == vt_sizeof) {
    str_append_chars(fc->tkn_buffer, value->item);
  } else if (value->type == vt_cast) {
    ValueCast* cast = value->item;
    str_append_chars(fc->tkn_buffer, "(");
    fc_write_c_type(fc->tkn_buffer, cast->as_type);
    str_append_chars(fc->tkn_buffer, ")");
    fc_write_c_value(fc, cast->value);
  } else if (value->type == vt_getptrv) {
    ValueCast* cast = value->item;
    str_append_chars(fc->tkn_buffer, "*(");
    fc_write_c_type(fc->tkn_buffer, cast->as_type);
    str_append_chars(fc->tkn_buffer, "*)");
    fc_write_c_value(fc, cast->value);
  } else if (value->type == vt_setptrv) {
    SetPtrValue* cast = value->item;
    str_append_chars(fc->tkn_buffer, "*(");
    fc_write_c_type(fc->tkn_buffer, cast->to_value->return_type);
    str_append_chars(fc->tkn_buffer, "*)");
    fc_write_c_value(fc, cast->ptr_value);
    str_append_chars(fc->tkn_buffer, " = ");
    fc_write_c_value(fc, cast->to_value);
  } else if (value->type == vt_class_init) {
    // Generate function
    GEN_C++;
    ValueClassInit* ini = value->item;
    Class* class = ini->class;
    char* func_name = malloc(30);
    sprintf(func_name, "_KI_CLASS_INIT_%d", GEN_C);
    str_append_chars(fc->tkn_buffer, func_name);
    str_append_chars(fc->tkn_buffer, "(");
    for (int i = 0; i < ini->prop_values->values->length; i++) {
      if (i > 0) {
        str_append_chars(fc->tkn_buffer, ", ");
      }
      Value* val = array_get_index(ini->prop_values->values, i);
      fc_write_c_value(fc, val);
    }
    str_append_chars(fc->tkn_buffer, ")");

    // Write header
    fc_write_c_type(fc->h_code, value->return_type);
    str_append_chars(fc->h_code, " ");
    str_append_chars(fc->h_code, func_name);
    str_append_chars(fc->h_code, "(");
    for (int i = 0; i < ini->prop_values->keys->length; i++) {
      if (i > 0) {
        str_append_chars(fc->h_code, ", ");
      }
      char* prop_name = array_get_index(ini->prop_values->keys, i);
      ClassProp* prop = map_get(class->props, prop_name);
      fc_write_c_type(fc->h_code, prop->return_type);
      str_append_chars(fc->h_code, " ");
      str_append_chars(fc->h_code, prop_name);
    }
    str_append_chars(fc->h_code, ");\n");

    // Write func
    fc_write_c_type(fc->c_code_after, value->return_type);
    str_append_chars(fc->c_code_after, " ");
    str_append_chars(fc->c_code_after, func_name);
    str_append_chars(fc->c_code_after, "(");
    for (int i = 0; i < ini->prop_values->keys->length; i++) {
      if (i > 0) {
        str_append_chars(fc->c_code_after, ", ");
      }
      char* prop_name = array_get_index(ini->prop_values->keys, i);
      ClassProp* prop = map_get(class->props, prop_name);
      fc_write_c_type(fc->c_code_after, prop->return_type);
      str_append_chars(fc->c_code_after, " ");
      str_append_chars(fc->c_code_after, prop_name);
    }
    str_append_chars(fc->c_code_after, ") {\n");
    char* sign;
    if (value->return_type->is_pointer) {
      sign = "->";
      fc_write_c_type(fc->c_code_after, value->return_type);
      str_append_chars(fc->c_code_after, " KI_RET_V = malloc(sizeof(");
      fc_write_c_type(fc->c_code_after, value->return_type);
      str_append_chars(fc->c_code_after, "));\n");
    } else {
      sign = ".";
      fc_write_c_type(fc->c_code_after, value->return_type);
      str_append_chars(fc->c_code_after, " KI_RET_V;\n");
    }

    for (int i = 0; i < ini->prop_values->keys->length; i++) {
      char* prop_name = array_get_index(ini->prop_values->keys, i);
      str_append_chars(fc->c_code_after, "KI_RET_V");
      str_append_chars(fc->c_code_after, sign);
      str_append_chars(fc->c_code_after, prop_name);
      str_append_chars(fc->c_code_after, " = ");
      str_append_chars(fc->c_code_after, prop_name);
      str_append_chars(fc->c_code_after, ";\n");
    }

    str_append_chars(fc->c_code_after, "return KI_RET_V;\n");
    str_append_chars(fc->c_code_after, "}\n\n");

  } else if (value->type == vt_prop_access) {
    ValueClassPropAccess* pa = value->item;
    if (pa->is_static) {
      Class* class = pa->on;
      ClassProp* prop = map_get(class->props, pa->name);
      // func ref
      Type* type = prop->return_type;
      Function* func = type->func;
      str_append_chars(fc->tkn_buffer, func->cname);
    } else {
      fc_write_c_value(fc, pa->on);
      Value* val = pa->on;
      Type* type = val->return_type;
      if (type->is_pointer) {
        str_append_chars(fc->tkn_buffer, "->");
      } else {
        str_append_chars(fc->tkn_buffer, ".");
      }
      str_append_chars(fc->tkn_buffer, pa->name);
    }
  } else {
    fc_error(fc, "Unhandled value token (compiler bug)", NULL);
  }
}

void fc_write_c_type(Str* append_to, Type* type) {
  if (type == NULL) {
    str_append_chars(append_to, "void");
    return;
  }
  //
  if (type->type == type_bool) {
    str_append_chars(append_to, "bool");
    return;
  }
  //
  if (type->type == type_void_pointer) {
    str_append_chars(append_to, "void*");
    return;
  }
  //
  if (type->type == type_struct) {
    Class* class = type->class;
    if (class->is_number) {
      int bytes = type->bytes;
      if (type->is_unsigned) {
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
      str_append_chars(append_to, "struct ");
      str_append_chars(append_to, class->cname);
    }

    if (type->is_pointer) {
      str_append_chars(append_to, "*");
    }
  }
}

void fc_write_c_if(FileCompiler* fc, TokenIf* ift) {
  //
  if (ift->is_else) {
    str_append_chars(fc->tkn_buffer, " else ");
  }
  if (ift->condition) {
    str_append_chars(fc->tkn_buffer, "if (");
    fc_write_c_value(fc, ift->condition);
    str_append_chars(fc->tkn_buffer, ") ");
  }
  str_append_chars(fc->tkn_buffer, "{\n");
  fc_write_c_ast(fc, ift->scope->ast);
  str_append_chars(fc->tkn_buffer, "}\n\n");

  if (ift->next) {
    fc_write_c_if(fc, ift->next);
  }
}