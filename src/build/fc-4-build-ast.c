
#include "../all.h"

void fc_build_asts() {
  //
  for (int i = 0; i < packages->keys->length; i++) {
    PkgCompiler* pkc = array_get_index(packages->values, i);

    for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
      FileCompiler* fc = array_get_index(pkc->file_compilers->values, o);

      // Functions
      for (int x = 0; x < fc->functions->length; x++) {
        Function* func = array_get_index(fc->functions, x);
        FileCompiler* fc = func->fc;
        if (fc->is_header) continue;
        fc_build_ast(fc, func->scope);
        Token* t = init_token();
        t->type = tkn_func;
        t->item = func;
        array_push(fc->scope->ast, t);
      }

      // Classes
      for (int x = 0; x < fc->classes->length; x++) {
        Class* class = array_get_index(fc->classes, x);
        FileCompiler* fc = class->fc;
        if (fc->is_header) continue;
        for (int y = 0; y < class->props->values->length; y++) {
          char* name = array_get_index(class->props->keys, y);
          ClassProp* prop = array_get_index(class->props->values, y);
          if (prop->is_func) {
            Function* func = prop->return_type->func;
            fc_build_ast(fc, func->scope);
            Token* t = init_token();
            t->type = tkn_func;
            t->item = func;
            array_push(fc->scope->ast, t);
          }
        }
      }

      //
    }
  }
}

void fc_build_ast(FileCompiler* fc, Scope* scope) {
  //
  char* token = malloc(KI_TOKEN_MAX);
  fc->i = scope->body_i;

  while (fc->i < fc->content_len) {
    fc_next_token(fc, token, false, false, true);

    if (strcmp(token, "}") == 0) {
      break;
    }

    if (strcmp(token, "#") == 0) {
      fc_parse_macro(fc, token);
      continue;
    }

    if (strcmp(token, "return") == 0) {
      token_return(fc, scope);
      continue;
    }

    if (strcmp(token, "if") == 0) {
      TokenIf* ift = token_if(fc, scope, false, true);
      Token* tk = init_token();
      tk->type = tkn_if;
      tk->item = ift;
      array_push(scope->ast, tk);
      continue;
    }

    if (strcmp(token, "while") == 0) {
      token_while(fc, scope);
      continue;
    }

    if (strcmp(token, "throw") == 0) {
      token_throw(fc, scope);
      continue;
    }

    if (strcmp(token, "free") == 0) {
      token_free(fc, scope);
      continue;
    }

    if (scope->in_loop) {
      if (strcmp(token, "break") == 0) {
        token_break(fc, scope);
        continue;
      }

      if (strcmp(token, "continue") == 0) {
        token_continue(fc, scope);
        continue;
      }
    }

    if (strcmp(token, "@") == 0) {
      token_declare(fc, scope, NULL);
      continue;
    }

    if (strcmp(token, "?") == 0) {
      fc->i--;
      Type* type = fc_read_type(fc);
      token_declare(fc, scope, type);
      continue;
    }

    // Identifier
    fc->i -= strlen(token);
    int current_i = fc->i;
    IdentifierFor* idf = fc_read_and_get_idf(fc, scope, false, true, true);

    // Check if declare
    if (idf && (idf->type == idfor_class || idf->type == idfor_enum) &&
        fc_get_char(fc, 0) == ' ') {
      fc_next_token(fc, token, true, true, true);
      if (is_valid_varname(token)) {
        // Var declaration
        fc->i = current_i;
        Type* type = fc_read_type(fc);
        token_declare(fc, scope, type);
        continue;
      }
    }

    // Value
    fc->i = current_i;
    Value* value = fc_read_value(fc, scope, false, true, true);

    // Check if assign
    fc_next_token(fc, token, true, true, true);
    if ((value->type == vt_var || value->type == vt_prop_access) &&
        (strcmp(token, "=") == 0 || strcmp(token, "-=") == 0 ||
         strcmp(token, "+=") == 0 || strcmp(token, "*=") == 0 ||
         strcmp(token, "/=") == 0 || strcmp(token, "\%=") == 0)) {
      //
      if (value->type == vt_prop_access) {
        ValueClassPropAccess* pa = value->item;
        if (pa->is_static || value->return_type->type == type_funcref) {
          fc_error(fc, "Cannot assign value to this", NULL);
        }
      }

      fc_next_token(fc, token, false, true, true);
      token_assign(fc, scope, token, value);
      fc_expect_token(fc, ";", false, true, true);
      continue;
    }

    // Plain value
    Token* t = init_token();
    t->type = tkn_value;
    t->item = value;

    if (value->return_type != NULL) {
      fc_error(fc,
               "Statement that returns a value, but has no variable to store "
               "it in",
               NULL);
    }

    array_push(scope->ast, t);

    fc_expect_token(fc, ";", false, true, true);

    continue;
    // fc_error(fc, "Unexpected token: '%s'\n", token);
  }

  if (scope->must_return && !scope->did_return) {
    fc_error(fc, "Scope is missing a return statement.", NULL);
  }

  free(token);
}
