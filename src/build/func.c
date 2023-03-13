
#include "../all.h"

Func *func_init(Allocator *alc) {
    //
    Func *func = al(alc, sizeof(Func));
    func->rett = NULL;
    func->args = array_make(alc, 8);
    func->args_by_name = map_make(alc);
    func->act = 0;
    func->chunk_args = NULL;
    func->chunk_body = NULL;

    func->is_static = false;
    func->is_generated = false;
    func->call_derefs = true;

    func->errors = NULL;
    func->can_error = false;

    return func;
}

void fcall_type_check(Fc *fc, Value *on, Array *values) {

    Array *args = on->rett->func_args;

    if (!args) {
        sprintf(fc->sbuf, "Trying to call a function on a non-function value");
        fc_error(fc);
    }

    if (values->length > args->length) {
        sprintf(fc->sbuf, "Too many arguments");
        fc_error(fc);
    }
    if (values->length < args->length) {
        sprintf(fc->sbuf, "Missing arguments");
        fc_error(fc);
    }

    for (int i = 0; i < args->length; i++) {
        //
        Arg *arg = array_get_index(args, i);
        Value *val = array_get_index(values, i);

        type_check(fc, arg->type, val->rett);
    }
}

void func_make_arg_decls(Func *func) {
    //
    Allocator *alc = func->fc->alc;
    Scope *fscope = func->scope;

    for (int i = 0; i < func->args->length; i++) {
        Arg *arg = array_get_index(func->args, i);

        Decl *decl = decl_init(alc, func->scope, arg->name, arg->type, NULL, arg->is_mut, true, false);

        Var *var = var_init(alc, decl, arg->type);

        Idf *idf = idf_init(alc, idf_var);
        idf->item = var;

        map_set(func->scope->identifiers, arg->name, idf);

        arg->decl = decl;

        if (func->call_derefs) {
            if (!fscope->decls)
                fscope->decls = array_make(alc, 8);
            array_push(fscope->decls, decl);
        }
    }
}
