
#include "../all.h"

Scope *scope_init(Allocator *alc, int type, Scope *parent, bool has_ast) {
    //
    Scope *scope = al(alc, sizeof(Scope));
    scope->parent = parent;
    scope->type = type;
    scope->identifiers = map_make(alc);
    scope->func = NULL;
    scope->ast = has_ast ? array_make(alc, 10) : NULL;
    scope->lvars = NULL;
    scope->var_info = map_make(alc);
    scope->did_return = false;
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
