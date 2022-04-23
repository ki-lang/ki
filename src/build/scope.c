
#include "../all.h"

Scope *init_scope() {
    Scope *scope = malloc(sizeof(Scope));
    scope->type = sct_unknown;
    scope->identifiers = map_make();
    scope->is_func = false;
    scope->is_loop = false;
    scope->in_loop = false;
    scope->must_return = false;
    scope->did_return = false;
    scope->catch_errors = false;
    scope->autofill_return_type = false;
    scope->class = NULL;
    scope->func = NULL;
    scope->body_i = 0;
    scope->body_i_end = 0;
    scope->ast = array_make(4);
    scope->parent = NULL;
    scope->return_type = NULL;
    //
    scope->var_bufs = array_make(8);
    scope->local_var_names = array_make(8);
    //
    return scope;
}

void free_scope(Scope *scope) {
    //
    free(scope);
}

Scope *init_sub_scope(Scope *parent) {
    Scope *scope = init_scope();
    scope->parent = parent;
    scope->in_loop = parent->in_loop;
    return scope;
}

Scope *get_class_scope(Scope *scope) {
    //
    Scope *class_scope = scope;
    while (class_scope && class_scope->type != sct_class) {
        class_scope = class_scope->parent;
    }
    return class_scope;
}
