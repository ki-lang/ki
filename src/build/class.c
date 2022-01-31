
#include "../all.h"

Class* init_class() {
  Class* class = malloc(sizeof(Class));
  class->name = NULL;
  class->fc = NULL;
  class->norfc = false;
  class->is_number = false;
  class->is_float = false;
  class->is_unsigned = false;
  class->size = 0;
  class->props = map_make();
  class->traits = array_make(2);
  return class;
}

void free_class(Class* class) {
  //
  free(class->name);
  // todo: free props
  //
  free(class);
}

ClassProp* init_class_prop() {
  ClassProp* prop = malloc(sizeof(ClassProp));
  prop->access_type = acct_public;
  prop->is_static = false;
  prop->is_func = false;
  prop->return_type = NULL;
  prop->default_value = NULL;
  prop->value_i = 0;
  prop->func = NULL;
  return prop;
}

void free_class_prop(ClassProp* prop) {
  //
  free_type(prop->return_type);
  //
  free(prop);
}

void fc_scan_class(FileCompiler* fc, Class* class) {
  //
  fc_expect_token(fc, "{", false, true, true);
  class->body_i = fc->i;
  // Look for class enums
  char* token = malloc(KI_TOKEN_MAX);

  while (fc->i < fc->content_len) {
    fc_next_token(fc, token, false, false, true);

    if (token[0] == '\0') {
      fc_error(fc, "Unexpected end of file, expected a '}' to close the class",
               NULL);
    }

    if (strcmp(token, "}") == 0) {
      break;
    }

    if (strcmp(token, "enum") == 0) {
      fc_next_token(fc, token, false, true, true);
      fc_name_taken(fc, class->props, token);

      Enum* enu = init_enum();
      enu->name = strdup(token);

      ClassProp* prop = init_class_prop();
      prop->access_type = acct_public;
      prop->is_static = true;
      Type* type = init_type();
      type->type = type_enum;
      type->enu = enu;
      prop->return_type = type;

      map_set(class->props, enu->name, prop);

      fc_read_enum_values(fc, enu);

      continue;
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

void fc_scan_class_props(Class* class) {
  //
  char* token = malloc(KI_TOKEN_MAX);
  FileCompiler* fc = class->fc;
  fc->i = class->body_i;
  //
  while (fc->i < class->body_i_end) {
    fc_next_token(fc, token, false, false, true);

    if (token[0] == '\0') {
      break;
    }

    if (strcmp(token, "}") == 0) {
      break;
    }

    // Skip enum
    if (strcmp(token, "enum") == 0) {
      fc_skip_until_char(fc, ';');
      fc_expect_token(fc, ";", false, true, false);
      continue;
    }

    if (strcmp(token, "trait") == 0) {
      fc_skip_until_char(fc, ';');
      fc_expect_token(fc, ";", false, true, false);
      continue;
    }

    int acc_type = acct_public;
    if (strcmp(token, "public") == 0) {
      fc_next_token(fc, token, false, true, true);
    }
    if (strcmp(token, "private") == 0) {
      fc_next_token(fc, token, false, true, true);
      acc_type = acct_private;
    }
    if (strcmp(token, "readonly") == 0) {
      fc_next_token(fc, token, false, true, true);
      acc_type = acct_readonly;
    }

    bool is_static = false;
    if (strcmp(token, "static") == 0) {
      fc_next_token(fc, token, false, true, true);
      is_static = true;
    }

    ClassProp* prop = init_class_prop();
    prop->access_type = acc_type;
    prop->is_static = is_static;

    if (strcmp(token, "func") == 0) {
      fc_next_token(fc, token, false, true, true);

      char* name = strdup(token);

      fc_name_taken(fc, class->props, name);

      Function* func = init_func();
      func->fc = fc;
      func->scope = init_scope();
      func->scope->parent = fc->scope;
      func->scope->is_func = true;

      Type* type = init_type();
      type->type = type_funcref;

      prop->return_type = type;
      prop->is_func = true;
      prop->func = func;

      map_set(class->props, name, prop);

      char* ctmp = malloc(strlen(class->name) + strlen(token) + 3);
      strcpy(ctmp, class->name);
      strcat(ctmp, "__");
      strcat(ctmp, name);

      char* cname = create_c_identifier_with_strings(fc->nsc->pkc->name,
                                                     fc->nsc->name, ctmp);

      IdentifierFor* find = map_get(c_identifiers, cname);
      if (find != NULL) {
        fc_error(fc,
                 "The function its exportable name is the same as a previous "
                 "declaration: '%s'",
                 cname);
      }

      IdentifierFor* idf = init_idf();
      idf->type = idfor_func;
      idf->item = func;

      map_set(c_identifiers, cname, idf);
      func->cname = cname;

      if (!prop->is_static) {
        // first arg is "this"
        FunctionArg* arg = init_func_arg();
        arg->name = "this";
        arg->type = fc_identifier_to_type(
            fc, create_identifier(class->fc->nsc->pkc->name,
                                  class->fc->nsc->name, class->name));

        array_push(func->args, arg);
        IdentifierFor* thisidf = init_idf();
        thisidf->type = idfor_var;
        thisidf->item = arg->type;
        map_set(func->scope->identifiers, "this", thisidf);
      }

      fc_expect_token(fc, "(", false, true, true);
      func->args_i = fc->i;
      func->args_i_end = fc->i + 5000;

      fc_scan_func_args(func);

      type->func_arg_types = func->args;
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
    Type* type = fc_read_type(fc);
    prop->return_type = type;

    class->size += type->bytes;

    fc_next_token(fc, token, false, true, true);
    char* name = strdup(token);
    fc_name_taken(fc, class->props, name);
    map_set(class->props, name, prop);

    // printf("p:%s\n", name);
    // if (type->class) {
    //   printf("c:%s\n", type->class->cname);
    // }
    // printf("---\n");

    fc_next_token(fc, token, true, true, true);
    if (strcmp(token, "=") == 0) {
      fc_next_token(fc, token, false, true, true);
      // prop->default_value = fc_read_value(fc, false, true, true);
      prop->value_i = fc->i;
      fc_skip_body(fc, "(", ")", ";", true);
    } else {
      fc_expect_token(fc, ";", false, true, true);
    }
  }
}