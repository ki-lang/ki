
#include "../all.h"

Decl *decl_init(Allocator *alc, Scope *scope, char *name, Type *type, Value *val, bool is_arg) {
    //
    Decl *v = al(alc, sizeof(Decl));
    v->name = name;
    v->type = type;
    v->value = val;
    v->scope = scope;
    v->is_mut = false;
    v->is_arg = is_arg;
    v->llvm_val = NULL;

    return v;
}

Var *var_init(Allocator *alc, Decl *decl, Type *type) {
    //
    Var *v = al(alc, sizeof(Var));
    v->decl = decl;
    v->type = type;
    return v;
}

Arg *arg_init(Allocator *alc, char *name, Type *type) {
    //
    Arg *v = al(alc, sizeof(Arg));
    v->name = name;
    v->type = type;
    v->is_mut = false;
    v->value = NULL;
    v->value_chunk = NULL;
    v->decl = NULL;
    v->type_chunk = NULL;

    return v;
}
