
#include "../all.h"

void fc_scan_types(FileCompiler* fc) {
  char* token = malloc(KI_TOKEN_MAX);
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
        fc_error(fc, "namespace token should be at the top of the file\n",
                 NULL);
      }
      fc_next_token(fc, token, false, true, true);
      // Set namespace
      fc->nsc = pkc_create_namespace(fc->nsc->pkc, token);
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
      fc_parse_macro(fc, token);

    } else if (strcmp(token, "enum") == 0) {
      fc_next_token(fc, token, false, true, true);
      fc_name_taken(fc, fc->nsc->scope->identifiers, token);

      Enum* enu = init_enum();
      enu->name = strdup(token);

      array_push(fc->enums, enu);

      fc_read_enum_values(fc, enu);

      IdentifierFor* idf = init_idf();
      idf->type = idfor_enum;
      idf->item = enu;

      Scope* scope = fc->nsc->scope;
      map_set(scope->identifiers, enu->name, idf);

      char* cname = create_c_identifier_with_strings(fc->nsc->pkc->name,
                                                     fc->nsc->name, enu->name);
      enu->cname = cname;
      map_set(c_identifiers, cname, idf);

    } else if (strcmp(token, "class") == 0) {
      fc_next_token(fc, token, false, true, true);
      fc_name_taken(fc, fc->nsc->scope->identifiers, token);

      char* name = strdup(token);

      Class* class = init_class();
      class->name = name;
      class->fc = fc;

      array_push(fc->classes, class);

      while (true) {
        fc_next_token(fc, token, true, true, true);

        if (strcmp(token, "norfc") == 0) {
          fc_next_token(fc, token, false, true, true);
          class->norfc = true;
          continue;
        }

        if (strcmp(token, "number") == 0 || strcmp(token, "float") == 0) {
          fc_next_token(fc, token, false, true, true);
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
            fc_error(fc, "Number byte size must be atleast 1, got: '%s'",
                     token);
          }
          continue;
        }
        break;
      }

      IdentifierFor* idf = init_idf();
      idf->type = idfor_class;
      idf->item = class;

      Scope* scope = fc->nsc->scope;
      map_set(scope->identifiers, name, idf);

      char* cname = create_c_identifier_with_strings(fc->nsc->pkc->name,
                                                     fc->nsc->name, name);
      class->cname = cname;
      map_set(c_identifiers, cname, idf);

      fc_scan_class(fc, class);

    } else if (strcmp(token, "func") == 0) {
      fc_next_token(fc, token, false, true, true);
      fc_name_taken(fc, fc->nsc->scope->identifiers, token);

      char* name = strdup(token);

      Function* func = init_func();
      func->fc = fc;
      func->scope = init_scope();
      func->scope->parent = fc->scope;
      func->scope->is_func = true;

      array_push(fc->functions, func);

      fc_scan_func(fc, func);

      IdentifierFor* idf = init_idf();
      idf->type = idfor_func;
      idf->item = func;

      Scope* scope = fc->nsc->scope;
      map_set(scope->identifiers, name, idf);

      char* cname = create_c_identifier_with_strings(fc->nsc->pkc->name,
                                                     fc->nsc->name, name);
      func->cname = cname;
      map_set(c_identifiers, cname, idf);

    } else if (strcmp(token, "use") == 0) {
      fc_next_token(fc, token, false, true, true);
      if (!is_valid_varname(token)) {
        fc_error(fc, "Invalid name: '%s'", token);
      }
      char* package = NULL;
      char* namespace = strdup(token);
      if (fc_get_char(fc, 0) == ':') {
        fc_next_token(fc, token, false, true, false);
        if (!is_valid_varname(token)) {
          fc_error(fc, "Invalid name: '%s'", token);
        }
        package = namespace;
        char* namespace = strdup(token);
      }

      PkgCompiler* pkc = fc->nsc->pkc;
      if (package != NULL) {
        pkc = pkc_get_by_name(package);
      }
      NsCompiler* nsc = pkc_create_namespace(pkc, namespace);
      free(package);
      free(namespace);

      fc_expect_token(fc, "{", false, true, true);
      fc_next_token(fc, token, false, false, true);
      while (strcmp(token, "}") != 0) {
        if (!is_valid_varname(token)) {
          fc_error(fc, "Invalid name: '%s'", token);
        }
        FcUse* fcu = malloc(sizeof(FcUse));
        fcu->pkc = pkc;
        fcu->nsc = nsc;

        map_set(fc->uses, strdup(token), fcu);

        fc_next_token(fc, token, false, false, true);
        if (strcmp(token, "}") == 0) {
          break;
        }
        fc_next_token(fc, token, false, false, true);
      }

      fc_expect_token(fc, ";", false, true, true);

    } else {
      fc_error(fc, "Unexpected token: '%s'", token);
    }
  }

  if (fc->macro_results->length > 0) {
    fc_error(
        fc, "Reached end of file, but some macro if statements were not closed",
        NULL);
  }

  free(token);
}
