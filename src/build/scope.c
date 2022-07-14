
#include "../all.h"

Scope *init_scope() {
    Scope *scope = malloc(sizeof(Scope));
    scope->type = sct_unknown;
    scope->identifiers = map_make();
    scope->is_func = false;
    scope->is_vscope = false;
    scope->is_loop = false;
    scope->in_loop = false;
    scope->must_return = false;
    scope->did_return = false;
    scope->catch_errors = false;
    scope->class = NULL;
    scope->func = NULL;
    scope->body_i = 0;
    scope->body_i_end = 0;
    scope->ast = array_make(4);
    scope->parent = NULL;
    //
    scope->vscope_vname = NULL;
    scope->vscope_return_type = NULL;
    //
    scope->var_bufs = array_make(8);
    scope->local_var_names = array_make(8);
    scope->llvm_declares = map_make(4);
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

Scope *get_func_scope(Scope *scope) {
    //
    Scope *func_scope = scope;
    while (func_scope && !func_scope->is_func) {
        func_scope = func_scope->parent;
    }
    return func_scope;
}

Scope *get_loop_scope(Scope *scope) {
    //
    Scope *loop_scope = scope;
    while (loop_scope && !loop_scope->is_loop) {
        loop_scope = loop_scope->parent;
    }
    return loop_scope;
}

Scope *get_vscope_scope(Scope *scope) {
    //
    Scope *vs_scope = scope;
    while (vs_scope && vs_scope->is_vscope == false) {
        vs_scope = vs_scope->parent;
    }
    return vs_scope;
}

void scope_remove_local_var_nullable(Scope *scope, LocalVar *lv) {
    // Create new type within scope
    Type *ntype = init_type();
    *ntype = *lv->type;
    ntype->nullable = false;

    // local idf
    IdentifierFor *idf = map_get(scope->identifiers, lv->name);
    LocalVar *nlv = lv;
    if (!idf) {
        // create local identifier
        nlv = malloc(sizeof(LocalVar));
        *nlv = *lv;

        idf = init_idf();
        idf->type = idfor_local_var;
        idf->item = nlv;
        map_set(scope->identifiers, lv->name, idf);
    }

    nlv->type = ntype;
}