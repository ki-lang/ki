
#include "../all.h"

void stage_1_func(Fc *fc);
void stage_1_class(Fc *fc);
void stage_1_enum(Fc *fc);

void stage_1(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 0) {
        printf("# Stage 1 : %s\n", fc->path_ki);
    }

    char *token = fc->token;

    while (true) {

        tok(fc, token, false, true);

        if (token[0] == 0)
            break;

        if (strcmp(token, "#") == 0) {
            read_macro(fc, fc->alc, fc->scope);
            continue;
        }

        if (strcmp(token, "func") == 0) {
            stage_1_func(fc);
            continue;
        }
        if (strcmp(token, "class") == 0) {
            stage_1_class(fc);
            continue;
        }

        sprintf(fc->sbuf, "Unexpected token '%s'", token);
        fc_error(fc);
    }

    //
    chain_add(b->stage_2, fc);
}

void stage_1_func(Fc *fc) {
    //
    char *token = fc->token;
    tok(fc, token, true, true);

    if (!is_valid_varname(token)) {
        sprintf(fc->sbuf, "Invalid function name syntax '%s'", token);
        fc_error(fc);
    }
    name_taken_check(fc, fc->nsc->scope, token);

    char *name = dups(fc->alc, token);
    char *gname = nsc_gname(fc->nsc, name);
    char *dname = nsc_dname(fc->nsc, name);

    Func *func = func_init(fc->alc);
    func->name = name;
    func->gname = gname;
    func->dname = dname;
    func->scope = scope_init(fc->alc, sct_func, fc->scope);
    func->scope->func = func;

    Idf *idf = idf_init(fc->alc, idf_func);
    idf->item = func;

    if (fc->is_header) {
        map_set(fc->scope->identifiers, name, idf);
    } else {
        map_set(fc->nsc->scope->identifiers, name, idf);
    }

    if (strcmp(func->gname, "main") == 0) {
        fc->b->main_func = func;
    }

    tok_expect(fc, "(", true, true);

    func->chunk_args = chunk_clone(fc->alc, fc->chunk);

    skip_body(fc, ')');

    if (fc->is_header) {
        skip_until_char(fc, ';');
    } else {
        skip_until_char(fc, '{');
        func->chunk_body = chunk_clone(fc->alc, fc->chunk);
        skip_body(fc, '}');
    }
}

void stage_1_class(Fc *fc) {
    //
    char *token = fc->token;
    tok(fc, token, true, true);

    if (!is_valid_varname(token)) {
        sprintf(fc->sbuf, "Invalid class name syntax '%s'", token);
        fc_error(fc);
    }
    name_taken_check(fc, fc->nsc->scope, token);

    char *name = dups(fc->alc, token);
    char *gname = nsc_gname(fc->nsc, name);
    char *dname = nsc_dname(fc->nsc, name);

    Class *class = class_init(fc->alc);
    class->name = name;
    class->gname = gname;
    class->dname = dname;
    class->scope = scope_init(fc->alc, sct_func, fc->scope);

    Idf *idf = idf_init(fc->alc, idf_class);
    idf->item = class;

    if (fc->is_header) {
        map_set(fc->scope->identifiers, name, idf);
    } else {
        map_set(fc->nsc->scope->identifiers, name, idf);
    }
}

void stage_1_enum(Fc *fc) {
    //
    char *token = fc->token;
    tok(fc, token, true, true);

    if (!is_valid_varname(token)) {
        sprintf(fc->sbuf, "Invalid enum name syntax '%s'", token);
        fc_error(fc);
    }
}
