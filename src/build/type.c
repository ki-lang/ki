
#include "../all.h"

Type* init_type() {
  Type* type = malloc(sizeof(Type));
  type->type = type_unknown;
  type->nullable = false;
  type->npt = false;
  type->allow_math = false;
  type->is_float = false;
  type->is_unsigned = false;
  type->is_pointer = false;
  type->is_array = false;
  //
  type->c_inline = false;
  type->c_static = false;
  //
  type->array_size = 0;
  type->bytes = 0;
  type->class = NULL;
  type->enu = NULL;
  //
  type->func_arg_types = NULL;
  type->func_return_type = NULL;
  type->func_can_error = false;
  return type;
}

void free_type(Type* type) {
  //
  free(type);
}

Type* fc_read_type(FileCompiler* fc) {
  //
  char* token = malloc(KI_TOKEN_MAX);
  bool nullable = false;
  bool npt = false;
  //
  fc_next_token(fc, token, false, false, true);
  if (strcmp(token, "?") == 0) {
    nullable = true;
    fc_next_token(fc, token, false, false, true);
  }

  if (strcmp(token, "NPT") == 0) {
    npt = true;
    fc_next_token(fc, token, false, false, true);
    if (strcmp(token, "?") == 0) {
      nullable = true;
    }
  }

  if (npt && nullable) {
    fc_error(fc, "NPT (non pointer type) cannot be NULL", NULL);
  }

  //
  if (!is_valid_varname(token)) {
    fc_error(fc, "Invalid type syntax: '%s'", token);
  }

  //
  fc->i -= strlen(token);
  Identifier* id = fc_read_identifier(fc, false, true, false);

  Type* t = fc_identifier_to_type(fc, id);
  if (t == NULL) {
    fc_error(fc, "Unknown type/class/enum: '%s'", token);
  }

  // enum
  if (t->class) {
    if (fc_get_char(fc, 0) == '.') {
      // Enum type
      fc->i++;
      fc_next_token(fc, token, false, true, false);
      Class* class = t->class;
      ClassProp* prop = map_get(class->props, token);
      if (!prop) {
        fc_error(fc, "Property '%s' not found on this class", token);
      }
      if (prop->return_type->type != type_enum) {
        fc_error(fc, "Property '%s' cannot be used as a type", token);
      }
      free(t);
      t = prop->return_type;
    }
  }

  //
  t->nullable = nullable;
  t->npt = npt;

  if (t->npt && t->type != type_struct) {
    fc_error(fc,
             "NPT (not pointer type) can only be applied to class instances",
             NULL);
  }

  if (t->nullable && !t->is_pointer) {
    fc_error(fc, "Invalid type, only pointer types can be null: '%s'", token);
  }

  if (t->npt) {
    t->is_pointer = false;
  }

  // Check if array
  fc_next_token(fc, token, true, true, false);
  if (strcmp(token, "[") == 0) {
    t->is_array = true;
    fc_next_token(fc, token, false, true, false);
    fc_next_token(fc, token, false, true, true);
    if (!is_valid_number(token)) {
      fc_error(fc, "Invalid number: '%s'", token);
    }
    int size = atoi(token);
    t->array_size = size;
    fc_expect_token(fc, "]", false, true, true);
  }

  free_id(id);
  free(token);
  return t;
}

Type* fc_identifier_to_type(FileCompiler* fc, Identifier* id) {
  Type* t = init_type();

  Scope* scope = fc->scope;

  PkgCompiler* pkc = fc->nsc->pkc;
  if (id->package != NULL) {
    pkc = pkc_get_by_name(id->package);
  }
  NsCompiler* nsc = fc->nsc;
  if (id->namespace != NULL) {
    nsc = pkc_get_namespace_by_name(pkc, id->namespace);
    scope = nsc->scope;
  }

  // Read standard type
  if (id->namespace == NULL) {
    if (strcmp(id->name, "void") == 0) {
      t->type = type_void;
    } else if (strcmp(id->name, "funcref") == 0) {
      char* token = malloc(KI_TOKEN_MAX);
      t->type = type_funcref;
      fc_expect_token(fc, "(", false, true, true);
      // Read arg types
      t->func_arg_types = array_make(4);
      fc_next_token(fc, token, true, true, true);
      while (strcmp(token, ")") != 0) {
        Type* arg_type = fc_read_type(fc);
        array_push(t->func_arg_types, arg_type);
        fc_next_token(fc, token, true, true, true);
        if (strcmp(token, ",") == 0) {
          fc_next_token(fc, token, false, true, true);
          fc_next_token(fc, token, true, true, true);
        } else {
          fc_expect_token(fc, ")", false, true, true);
        }
      }

      // Read return type
      fc_next_token(fc, token, true, true, true);
      if (strcmp(token, "!") == 0) {
        t->func_can_error = true;
        fc_next_token(fc, token, false, true, true);
      }
      t->func_return_type = fc_read_type(fc);

      free(token);
    }
  }

  if (id->package && strcmp(id->package, "ki") == 0 &&
      strcmp(id->namespace, "type") == 0) {
    if (strcmp(id->name, "ptr") == 0) {
      t->type = type_void_pointer;
      IdentifierFor* idf = idf_find_in_scope(scope, id->name);
      if (idf == NULL) {
        fc_error(fc, "Compiler bug, cant find ptr class", NULL);
      }
      t->is_pointer = true;
      t->class = idf->item;
      t->bytes = pointer_size;
      t->allow_math = true;
    }
    if (strcmp(id->name, "bool") == 0) {
      t->type = type_bool;
      IdentifierFor* idf = idf_find_in_scope(scope, id->name);
      if (idf == NULL) {
        fc_error(fc, "Compiler bug, cant find ptr class", NULL);
      }
      t->class = idf->item;
      t->bytes = pointer_size;
    }
  }

  if (t->type == type_unknown) {
    // Check other types
    IdentifierFor* idf = idf_find_in_scope(scope, id->name);

    if (idf == NULL) {
      free_type(t);
      return NULL;
    }

    if (idf->type == idfor_enum) {
      t->type = type_enum;
      t->enu = idf->item;
      t->bytes = pointer_size;
    } else if (idf->type == idfor_class) {
      t->type = type_struct;
      t->class = idf->item;
      t->is_pointer = true;
      t->bytes = pointer_size;
      if (t->class->is_number) {
        // t->type = type_number;
        t->is_pointer = false;
        t->allow_math = true;
        t->bytes = t->class->size;
      }
    }

    if (idf->type == idfor_class) {
      // Check for generics
      Class* class = t->class;
      if (class->generic_names != NULL) {
        // Find generic class
        char* fci = fc->i;
        char* uid = fc_class_read_generic_unique_id(fc);
        char* cname = malloc(KI_TOKEN_MAX);
        strcpy(cname, class->cname);
        strcat(cname, "__");
        strcat(cname, uid);

        Class* gclass = map_get(c_identifiers, cname);
        if (!gclass) {
          gclass = fc_make_generic_class(class);
          gclass->cname = strdup(cname);
          // Read generic types
          fc_expect_token(fc, "<", false, true, true);
          int generic_c = 0;
          while (generic_c < class->generic_names->length) {
            Type* gen_t = fc_read_type(fc);
            char* name = array_get_index(class->generic_names, generic_c);
            map_set(gclass->generic_types, name, gen_t);
            generic_c++;
            if (generic_c < class->generic_names->length) {
              fc_expect_token(fc, ",", false, true, true);
            }
          }
          fc_expect_token(fc, ">", false, true, true);

          // Scan class
          fc_scan_class_props(gclass, true);
          fc_scan_class_prop_values(gclass, true);
        }

        free(cname);

        t->class = gclass;
      }
    }
  }

  if (t->type == type_unknown) {
    free_type(t);
    return NULL;
  }

  return t;
}

Type* fc_create_type_for_enum(Enum* enu) {
  //
  Type* t = init_type();
  t->type = type_enum;
  t->enu = enu;
  t->bytes = pointer_size;
  return t;
}

void fc_type_compatible(FileCompiler* fc, Type* t1, Type* t2) {
  if (!type_compatible(t1, t2)) {
    fc_error(fc, "Types are not compatible", NULL);
  }
}

bool type_compatible(Type* t1, Type* t2) {
  //
  return true;
}
