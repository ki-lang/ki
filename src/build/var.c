
#include "../all.h"

Var *var_init(Allocator *alc, char *name, Type *type, bool is_mut, bool is_arg, bool is_global) {
    //
    Var *v = al(alc, sizeof(Var));
    v->name = name;
    v->type = type;
    v->is_mut = is_mut;
    v->is_arg = is_arg;
    v->is_global = is_global;

    return v;
}
