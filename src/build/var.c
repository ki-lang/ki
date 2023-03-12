
#include "../all.h"

Decl *decl_init(Allocator *alc, char *name, Type *type, Value *val, bool is_mut, bool is_arg, bool is_global) {
    //
    Decl *v = al(alc, sizeof(Decl));
    v->name = name;
    v->type = type;
    v->value = val;
    v->owner_passes = 0;
    v->is_mut = is_mut;
    v->is_arg = is_arg;
    v->is_global = is_global;

    return v;
}

Var *var_init(Allocator *alc, Decl *decl, Type *type) {
    //
    Var *v = al(alc, sizeof(Var));
    v->decl = decl;
    v->type = type;
    return v;
}

Arg *arg_init(Allocator *alc, char *name, Type *type, bool is_mut) {
    //
    Arg *v = al(alc, sizeof(Arg));
    v->name = name;
    v->type = type;
    v->is_mut = is_mut;
    v->value = NULL;
    v->value_chunk = NULL;

    return v;
}
