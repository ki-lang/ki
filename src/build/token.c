
#include "../all.h"

Token* init_token() {
  Token* t = malloc(sizeof(Token));
  t->type = tkn_unknown;
  t->item = NULL;
  return t;
}

void free_token(Token* token) {
  //
  free(token);
}

void token_return(FileCompiler* fc, Scope* scope) {
  //
  Token* t = init_token();
  t->type = tkn_return;

  Scope* func_scope = scope;
  while (func_scope->is_func == false && func_scope->parent != NULL) {
    func_scope = func_scope->parent;
  }

  if (!func_scope->is_func) {
    fc_error(fc, "Trying to return in a non function scope", NULL);
  }

  char* token = malloc(KI_TOKEN_MAX);
  fc_next_token(fc, token, true, true, true);

  if (strcmp(token, ";") != 0) {
    Value* value = fc_read_value(fc, scope, false, true, true);

    if (func_scope->autofill_return_type && func_scope->return_type == NULL) {
      func_scope->return_type = value->return_type;
    }

    if (func_scope->return_type == NULL) {
      fc_error(fc,
               "Function has no return type, but you are returning a value.",
               NULL);
    }

    fc_type_compatible(fc, func_scope->return_type, value->return_type);

    t->item = value;
  }

  fc_expect_token(fc, ";", false, true, true);

  free(token);

  func_scope->did_return = true;

  array_push(scope->ast, t);
}

TokenIf* token_if(FileCompiler* fc, Scope* scope, bool is_else,
                  bool has_condition) {
  //
  Value* value = NULL;
  //
  if (has_condition) {
    fc_expect_token(fc, "(", false, true, true);

    value = fc_read_value(fc, scope, false, true, true);
    if (value->return_type->type != type_bool) {
      fc_error(fc, "if statement must return a bool", NULL);
    }

    fc_expect_token(fc, ")", false, true, true);
  }

  fc_expect_token(fc, "{", false, true, true);

  TokenIf* ift = malloc(sizeof(TokenIf));
  ift->condition = value;
  ift->scope = init_sub_scope(scope);
  ift->scope->body_i = fc->i;
  ift->is_else = is_else;
  ift->next = NULL;

  fc_build_ast(fc, ift->scope);

  char* token = malloc(KI_TOKEN_MAX);
  fc_next_token(fc, token, true, false, true);

  if (strcmp(token, "else") == 0) {
    fc_next_token(fc, token, false, false, true);
    bool else_has_cond = false;
    fc_next_token(fc, token, true, true, true);
    if (strcmp(token, "if") == 0) {
      fc_next_token(fc, token, false, true, true);
      else_has_cond = true;
    }
    ift->next = token_if(fc, scope, true, else_has_cond);
  }

  free(token);

  return ift;
}

void token_while(FileCompiler* fc, Scope* scope) {
  //
  fc_expect_token(fc, "(", false, true, true);
  Value* value = fc_read_value(fc, scope, false, true, true);
  fc_expect_token(fc, ")", false, true, true);
  fc_expect_token(fc, "{", false, true, true);
  Scope* wscope = init_sub_scope(scope);
  wscope->body_i = fc->i;
  wscope->is_loop = true;
  wscope->in_loop = true;
  Token* t = init_token();
  t->type = tkn_while;
  TokenWhile* wt = malloc(sizeof(TokenWhile));
  wt->condition = value;
  wt->scope = wscope;
  t->item = wt;

  fc_build_ast(fc, wscope);

  array_push(scope->ast, t);
}

void token_throw(FileCompiler* fc, Scope* scope) {
  //
  Token* t = init_token();
  t->type = tkn_throw;
  char* msg = malloc(KI_TOKEN_MAX);
  fc_next_token(fc, msg, false, true, true);

  Scope* throw_scope = scope;
  while (throw_scope && throw_scope->is_func == false) {
    throw_scope = throw_scope->parent;
  }

  if (throw_scope == NULL) {
    fc_error(fc, "Trying to throw inside a non function space", NULL);
  }
  if (throw_scope->func->can_error == false) {
    fc_error(fc,
             "Trying to throw inside a function that cannot return an error",
             NULL);
  }

  TokenThrow* tt = malloc(sizeof(TokenThrow));
  tt->msg = msg;
  tt->return_type = throw_scope->return_type;

  t->item = tt;
  array_push(scope->ast, t);

  fc_expect_token(fc, ";", false, true, true);
}

void token_static(FileCompiler* fc, Scope* scope) {
  // Get type
  Type* left_type = fc_read_type(fc, scope);

  // Get var name
  char* token = malloc(KI_TOKEN_MAX);
  fc_next_token(fc, token, false, true, true);
  fc_name_taken(fc, scope->identifiers, token);

  //
  fc_expect_token(fc, "{", false, true, true);
  //
  Scope* val_scope = init_sub_scope(fc->scope);
  val_scope->body_i = fc->i;
  val_scope->is_func = true;
  val_scope->autofill_return_type = true;

  fc_build_ast(fc, val_scope);

  if (!left_type && !scope->return_type) {
    fc_error(fc, "Scope must return a value", NULL);
  }

  if (left_type) {
    if (!type_compatible(left_type, scope->return_type)) {
      fc_error(fc, "Types are not compatible", NULL);
    }
  }

  char* gname = malloc(KI_TOKEN_MAX);
  sprintf(gname, "_KI_STATIC_VAR_%d_%s", fc->static_vars->length, fc->hash);

  TokenStaticDeclare* decl = malloc(sizeof(TokenStaticDeclare));
  decl->name = token;
  decl->global_name = gname;
  decl->scope = val_scope;
  decl->type = left_type ? left_type : scope->return_type;

  Token* t = init_token();
  t->type = tkn_static;
  t->item = decl;

  array_push(scope->ast, t);
  array_push(fc->static_vars, decl);

  IdentifierFor* idf = init_idf();
  idf->type = idfor_static_var;
  idf->item = decl;

  map_set(scope->identifiers, decl->name, idf);
}

void token_set_threaded(FileCompiler* fc, Scope* scope) {
  Token* t = init_token();
  t->type = tkn_set_threaded;

  Identifier* id = fc_read_identifier(fc, false, true, true);
  Scope* idf_scope = fc_get_identifier_scope(fc, fc->scope, id);
  IdentifierFor* idf = idf_find_in_scope(idf_scope, id);

  if (!idf || idf->type != idfor_threaded_var) {
    fc_error(fc, "Cannot find threaded global variable: %s", id->name);
  }

  ThreadedGlobal* tg = idf->item;

  fc_expect_token(fc, "=", false, true, true);

  TokenIdValue* iv = malloc(sizeof(TokenIdValue));
  iv->name = fc_create_identifier_global_cname(fc, id);
  iv->value = fc_read_value(fc, scope, false, true, true);

  // Todo: type check value->return_type with tg->type

  t->item = iv;
  array_push(scope->ast, t);

  fc_expect_token(fc, ";", false, true, true);
}

void token_mutex_init(FileCompiler* fc, Scope* scope) {
  Value* val = fc_read_value(fc, scope, false, true, true);

  Token* t = init_token();
  t->type = tkn_mutex_init;
  t->item = val;

  array_push(scope->ast, t);
  fc_expect_token(fc, ";", false, true, true);
}

void token_mutex_lock(FileCompiler* fc, Scope* scope) {
  Value* val = fc_read_value(fc, scope, false, true, true);

  Token* t = init_token();
  t->type = tkn_mutex_lock;
  t->item = val;

  array_push(scope->ast, t);
  fc_expect_token(fc, ";", false, true, true);
}

void token_mutex_unlock(FileCompiler* fc, Scope* scope) {
  Value* val = fc_read_value(fc, scope, false, true, true);

  Token* t = init_token();
  t->type = tkn_mutex_unlock;
  t->item = val;

  array_push(scope->ast, t);
  fc_expect_token(fc, ";", false, true, true);
}

void token_break(FileCompiler* fc, Scope* scope) {
  //
  Token* t = init_token();
  t->type = tkn_break;
  array_push(scope->ast, t);

  fc_expect_token(fc, ";", false, true, true);
}

void token_continue(FileCompiler* fc, Scope* scope) {
  //
  Token* t = init_token();
  t->type = tkn_continue;
  array_push(scope->ast, t);
  fc_expect_token(fc, ";", false, true, true);
}

void token_free(FileCompiler* fc, Scope* scope) {
  //
  Token* t = init_token();
  t->type = tkn_free;
  t->item = fc_read_value(fc, scope, false, true, true);
  array_push(scope->ast, t);
  fc_expect_token(fc, ";", false, true, true);
}

void token_declare(FileCompiler* fc, Scope* scope, Type* left_type) {
  // Get var name
  char* token = malloc(KI_TOKEN_MAX);
  fc_next_token(fc, token, false, true, true);
  fc_name_taken(fc, scope->identifiers, token);

  //
  fc_expect_token(fc, "=", false, true, true);
  //
  Value* value = fc_read_value(fc, scope, false, true, true);

  if (left_type) {
    if (!type_compatible(left_type, value->return_type)) {
      fc_error(fc, "Types are not compatible", NULL);
    }
  }

  TokenDeclare* decl = malloc(sizeof(TokenDeclare));
  decl->name = token;
  decl->value = value;
  decl->type = left_type ? left_type : value->return_type;

  Token* t = init_token();
  t->type = tkn_declare;
  t->item = decl;

  array_push(scope->ast, t);

  IdentifierFor* idf = init_idf();
  idf->type = idfor_var;
  idf->item = decl->type;

  map_set(scope->identifiers, decl->name, idf);

  fc_expect_token(fc, ";", false, true, true);
}

void token_assign(FileCompiler* fc, Scope* scope, char* sign, Value* left) {
  //
  TokenAssign* ta = malloc(sizeof(TokenAssign));
  ta->left = left;

  if (strcmp(sign, "=") == 0) {
    ta->type = op_eq;
  } else if (strcmp(sign, "+=") == 0) {
    ta->type = op_add;
  } else if (strcmp(sign, "-=") == 0) {
    ta->type = op_sub;
  } else if (strcmp(sign, "*=") == 0) {
    ta->type = op_mult;
  } else if (strcmp(sign, "/=") == 0) {
    ta->type = op_div;
  } else if (strcmp(sign, "\%=") == 0) {
    ta->type = op_mod;
  } else {
    fc_error(fc, "Invalid assign token: '%s'", sign);
  }

  ta->right = fc_read_value(fc, scope, false, true, true);

  Token* t = init_token();
  t->type = tkn_assign;
  t->item = ta;

  array_push(scope->ast, t);
}