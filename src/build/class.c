
#include "../all.h"

Class* init_class() {
  Class* class = malloc(sizeof(Class));
  class->name = NULL;
  class->cname = NULL;
  class->fc = NULL;
  class->ref_count = true;
  class->is_number = false;
  class->is_float = false;
  class->is_unsigned = false;
  class->is_ctype = false;
  class->size = 0;
  class->props = map_make();
  class->traits = array_make(2);
  class->generic_names = NULL;
  class->generic_types = NULL;
  class->generic_hash = NULL;
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

Class* fc_make_generic_class(Class* class) {
  Class* gclass = malloc(sizeof(Class));
  memcpy(gclass, class, sizeof(Class));
  gclass->generic_types = map_make();
  gclass->props = map_make();
  gclass->traits = array_make(2);
  return gclass;
}

Class* fc_get_generic_class(FileCompiler* fc, Class* class) {
  int fci = fc->i;
  char* uid = fc_class_read_generic_unique_id(fc);
  char* cname = malloc(KI_TOKEN_MAX);
  strcpy(cname, class->cname);
  strcat(cname, "__");
  strcat(cname, uid);

  Class* gclass = NULL;
  IdentifierFor* idf = map_get(c_identifiers, cname);
  if (idf) {
    gclass = idf->item;
    free(uid);
  } else {
    gclass = fc_make_generic_class(class);
    gclass->cname = strdup(cname);
    gclass->generic_hash = uid;
    // Read generic types
    fc->i = fci;
    fc_expect_token(fc, "<", false, true, true);
    int generic_c = 0;
    while (generic_c < class->generic_names->length) {
      Type* gen_t = fc_read_type(fc);
      char* name = array_get_index(class->generic_names, generic_c);
      IdentifierFor* idf = init_idf();
      idf->type = idfor_type;
      idf->item = gen_t;
      map_set(gclass->generic_types, name, idf);
      generic_c++;
      if (generic_c < class->generic_names->length) {
        fc_expect_token(fc, ",", false, true, true);
      }
    }
    fc_expect_token(fc, ">", false, true, true);

    // Add to class lists
    array_push(gclass->fc->classes, gclass);

    IdentifierFor* idf = init_idf();
    idf->type = idfor_class;
    idf->item = gclass;

    map_set(c_identifiers, gclass->cname, idf);

    char* vn = malloc(KI_TOKEN_MAX);
    strcpy(vn, gclass->name);
    strcat(vn, "__");
    strcat(vn, uid);

    map_set(gclass->fc->nsc->scope->identifiers, vn, idf);
  }

  free(cname);

  return gclass;
}

Map* fc_class_set_generic_identifiers(Class* gclass) {
  //
  Map* fcidfs = gclass->fc->scope->identifiers;
  Map* prev_identifiers = map_make();
  for (int i = 0; i < gclass->generic_types->keys->length; i++) {
    char* key = array_get_index(gclass->generic_types->keys, i);
    IdentifierFor* tidf = array_get_index(gclass->generic_types->values, i);
    IdentifierFor* pidf = map_get(fcidfs, key);
    if (pidf) {
      map_set(prev_identifiers, key, pidf);
    }
    map_set(fcidfs, key, tidf);
  }

  return prev_identifiers;
}
Map* fc_class_restore_generic_identifiers(Class* gclass,
                                          Map* prev_identifiers) {
  // Set old identifiers
  Map* fcidfs = gclass->fc->scope->identifiers;
  //
  for (int i = 0; i < prev_identifiers->keys->length; i++) {
    char* key = array_get_index(prev_identifiers->keys, i);
    IdentifierFor* pidf = array_get_index(prev_identifiers->values, i);
    map_set(fcidfs, key, pidf);
  }
  //
  map_free(prev_identifiers, false);
}

char* fc_class_read_generic_unique_id(FileCompiler* fc) {
  //
  Str* uid = str_make("|");
  char* token = malloc(KI_TOKEN_MAX);

  fc_expect_token(fc, "<", false, true, true);

  fc_next_token(fc, token, true, true, true);
  while (strcmp(token, ">") != 0) {
    Identifier* id = fc_read_identifier(fc, false, true, true);
    char* gname = fc_create_identifier_global_cname(fc, id);
    str_append_chars(uid, gname);
    free(gname);

    fc_next_token(fc, token, true, true, true);
    if (strcmp(token, ">") != 0) {
      fc_expect_token(fc, ",", false, true, true);
      str_append_chars(uid, ",");
    }
  }

  fc_expect_token(fc, ">", false, true, true);
  str_append_chars(uid, "|");

  //
  char* uidchars = str_to_chars(uid);
  free_str(uid);

  char* hash = malloc(33);
  strcpy(hash, "");
  md5(uidchars, hash);

  free(uidchars);

  return hash;
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

    if (strcmp(token, "trait") == 0) {
      Identifier* id = fc_read_identifier(fc, false, true, true);
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
  if (class->generic_names != NULL && class->generic_hash == NULL) {
    return;
  }
  //
  Map* prev_identifiers = NULL;
  if (class->generic_hash) {
    prev_identifiers = fc_class_set_generic_identifiers(class);
  }
  //
  char* token = malloc(KI_TOKEN_MAX);
  FileCompiler* fc = class->fc;
  fc->i = class->body_i;

  Array* chunks = array_make(2);

  //
  while (true) {
    fc_next_token(fc, token, false, false, true);

    if (token[0] == '\0' || strcmp(token, "}") == 0) {
      if (chunks->length > 0) {
        ContentChunk* cc = content_chunk_pop(chunks);
        fc = cc->fc;
        fc->i = cc->i;
        continue;
      }
      break;
    }

    // Skip enum
    if (strcmp(token, "enum") == 0) {
      fc_skip_until_char(fc, ';');
      fc_expect_token(fc, ";", false, true, false);
      continue;
    }

    if (strcmp(token, "trait") == 0) {
      Identifier* id = fc_read_identifier(fc, false, true, true);
      Scope* idf_scope = fc_get_identifier_scope(fc, fc->scope, id);
      IdentifierFor* idf = idf_find_in_scope(idf_scope, id);
      if (!idf) {
        fc_error(fc, "Cannot find trait: %s", id->name);
      }
      if (idf->type != idfor_trait) {
        fc_error(fc, "Is not a trait: %s", id->name);
      }
      fc_expect_token(fc, ";", false, true, true);
      Trait* trait = idf->item;
      ContentChunk* cc = content_chunk_create_for_fc(chunks, fc);
      // Scope* prev_scope = fc->scope;
      fc = trait->fc;
      fc->i = trait->body_i;
      // fc->scope = prev_scope;
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

      char* cname = malloc(strlen(class->cname) + strlen(token) + 3);
      strcpy(cname, class->cname);
      strcat(cname, "__");
      strcat(cname, name);

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
        Identifier* fid = create_identifier(class->fc->nsc->pkc->name,
                                            class->fc->nsc->name, class->name);
        if (class->generic_hash) {
          fid->generic_hash = strdup(class->generic_hash);
        }
        FunctionArg* arg = init_func_arg();
        arg->name = "this";
        arg->type = fc_identifier_to_type(fc, fid);

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

      type->func_arg_types = array_make(4);
      for (int i = 0; i < func->args->length; i++) {
        FunctionArg* arg = array_get_index(func->args, i);
        array_push(type->func_arg_types, arg->type);
      }
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
    ClassProp* prop = init_class_prop();
    prop->access_type = acct_public;
    prop->is_static = false;

    Type* type =
        fc_identifier_to_type(fc, create_identifier("ki", "type", "u16"));
    prop->return_type = type;

    Value* def_value = init_value();
    def_value->type = vt_number;
    def_value->item = "0";

    prop->default_value = def_value;

    class->size += type->bytes;
    map_set(class->props, "_RC", prop);

    // Allocator
    prop = init_class_prop();
    prop->access_type = acct_public;
    prop->is_static = false;

    type =
        fc_identifier_to_type(fc, create_identifier("ki", "mem", "Allocator"));
    prop->return_type = type;

    // def_value = init_value();
    // def_value->type = vt_number;
    // def_value->item = "NULL";

    // prop->default_value = def_value;

    class->size += type->bytes;
    map_set(class->props, "_ALLOCATOR", prop);
  }

  if (prev_identifiers) {
    fc_class_restore_generic_identifiers(class, prev_identifiers);
  }
}

void fc_scan_class_prop_values(Class* class) {
  //
  if (class->generic_names != NULL && class->generic_hash == NULL) {
    return;
  }
  //
  Map* prev_identifiers = NULL;
  if (class->generic_hash) {
    prev_identifiers = fc_class_set_generic_identifiers(class);
  }

  FileCompiler* fc = class->fc;

  for (int o = 0; o < class->props->keys->length; o++) {
    //
    ClassProp* prop = array_get_index(class->props->values, o);
    //
    if (prop->value_i > 0) {
      fc->i = prop->value_i;
      prop->default_value = fc_read_value(fc, fc->scope, false, true, true);
      fc_expect_token(fc, ";", false, true, true);
    }
  }

  if (prev_identifiers) {
    fc_class_restore_generic_identifiers(class, prev_identifiers);
  }
}