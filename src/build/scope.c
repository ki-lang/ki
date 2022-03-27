
#include "../all.h"

Scope* init_scope() {
  Scope* scope = malloc(sizeof(Scope));
  scope->identifiers = map_make();
  scope->is_func = false;
  scope->in_loop = false;
  scope->must_return = false;
  scope->did_return = false;
  scope->catch_errors = false;
  scope->autofill_return_type = false;
  scope->body_i = 0;
  scope->body_i_end = 0;
  scope->ast = array_make(4);
  scope->parent = NULL;
  scope->return_type = NULL;
  return scope;
}

void free_scope(Scope* scope) {
  //
  free(scope);
}

Scope* init_sub_scope(Scope* parent) {
  Scope* scope = init_scope();
  scope->parent = parent;
  scope->in_loop = parent->in_loop;
  return scope;
}