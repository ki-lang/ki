
#include "../all.h"

Scope *scope_init(Allocator *alc, int type, Scope *parent) {
    //
    Scope *scope = al(alc, sizeof(Scope));
    scope->parent = parent;
    scope->type = type;
    scope->identifiers = map_make(alc);
    scope->func = NULL;
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
