
#include "../all.h"

Func *func_init(Allocator *alc, Build *b) {
    //
    Func *func = al(alc, sizeof(Func));
    func->rett = b->type_void;
    func->args = array_make(alc, 8);
    func->args_by_name = map_make(alc);
    func->act = 0;
    func->line = 0;
    func->chunk_args = NULL;
    func->chunk_body = NULL;
    func->test = NULL;

    func->is_static = false;
    func->is_generated = false;
    func->is_test = false;
    func->only_returns_strict = true;
    func->will_exit = false;
    func->uses_stack_alloc = false;
    func->parse_args = true;

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

        Decl *decl = decl_init(alc, fscope, arg->name, arg->type, NULL, true);
        Idf *idf = idf_init(alc, idf_decl);
        idf->item = decl;

        map_set(func->scope->identifiers, arg->name, idf);

        arg->decl = decl;

        usage_line_init(alc, fscope, decl);
    }
}

void *free_delayed_exec(void *item) {
    //
    sleep_ms(10000);
    free(item);
    return NULL;
}

void rtrim(char *str, char ch) {
    //
    int i = strlen(str) - 1;
    while (i >= 0) {
        char x = str[i];
        if (x == ch) {
            str[i] = '\0';
            i--;
            continue;
        }
        break;
    }
}

void free_delayed(void *item) {
#ifdef WIN32
    void *thr = CreateThread(NULL, 0, (unsigned long (*)(void *))free_delayed_exec, item, 0, NULL);
#else
    pthread_t thr;
    pthread_create(&thr, NULL, free_delayed_exec, item);
#endif
}
