
#include "../all.h"

Scope *scope_init(Allocator *alc, int type, Scope *parent, bool has_ast) {
    //
    Scope *scope = al(alc, sizeof(Scope));
    scope->parent = parent;
    scope->type = type;
    scope->identifiers = map_make(alc);
    scope->upref_slots = map_make(alc);
    scope->func = NULL;
    scope->ast = has_ast ? array_make(alc, 10) : NULL;
    scope->usage_keys = NULL;
    scope->usage_values = NULL;
    scope->vscope = NULL;

    scope->did_return = false;
    scope->in_loop = type == sct_loop;

    if (parent) {
        scope->func = parent->func;
        scope->in_loop = parent->in_loop;
    }

    return scope;
}

void name_taken_check(Fc *fc, Scope *scope, char *name) {
    //
    Idf *idf = map_get(scope->identifiers, name);
    if (idf) {
        sprintf(fc->sbuf, "Name already in use '%s'", name);
        fc_error(fc);
    }
}

Scope *scope_find(Scope *scope, int type) {
    while (scope && scope->type != type) {
        scope = scope->parent;
    }
    return scope;
}

Scope *scope_find_return_scope(Scope *scope) {
    while (scope && scope->type != sct_func && scope->type != sct_vscope) {
        scope = scope->parent;
    }
    return scope;
}

bool scope_contains(Scope *parent_scope, Scope *scope) {
    while (scope) {
        if (scope == parent_scope) {
            return true;
        }
        scope = scope->parent;
    }
    return false;
}
