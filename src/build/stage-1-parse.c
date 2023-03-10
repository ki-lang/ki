
#include "../all.h"

void stage_1_func(Fc *fc);
void stage_1_class(Fc *fc);
void stage_1_enum(Fc *fc);

void stage_1(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 1) {
        printf("# Stage 1 : Parse : %s\n", fc->path_ki);
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
    func->fc = fc;
    func->name = name;
    func->gname = gname;
    func->dname = dname;
    func->scope = scope_init(fc->alc, sct_func, fc->scope, true);
    func->scope->func = func;

    array_push(fc->funcs, func);

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
        skip_until_char(fc, ";");
    } else {
        skip_until_char(fc, "{");
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
    class->fc = fc;
    class->name = name;
    class->gname = gname;
    class->dname = dname;
    class->scope = scope_init(fc->alc, sct_func, fc->scope, false);

    array_push(fc->classes, class);

    Idf *idf = idf_init(fc->alc, idf_class);
    idf->item = class;

    if (fc->is_header) {
        map_set(fc->scope->identifiers, name, idf);
    } else {
        map_set(fc->nsc->scope->identifiers, name, idf);
    }

    map_set(class->scope->identifiers, "CLASS", idf);

    tok(fc, token, true, true);
    while (strcmp(token, "{") != 0) {
        if (strcmp(token, "type") == 0) {
            tok_expect(fc, ":", true, false);
            tok(fc, token, true, false);
            if (strcmp(token, "ptr") == 0) {
                class->type = ct_ptr;
                class->is_rc = false;
                class->size = fc->b->ptr_size;
            } else if (strcmp(token, "int") == 0 || strcmp(token, "float") == 0) {
                class->type = strcmp(token, "int") == 0 ? ct_int : ct_float;
                class->is_rc = false;
                tok_expect(fc, ":", true, false);
                tok(fc, token, true, false);
                int size = 0;
                if (strcmp(token, "*") == 0) {
                    size = fc->b->ptr_size;
                } else {
                    if (!is_valid_number(token)) {
                        sprintf(fc->sbuf, "Invalid number byte size: '%s'", token);
                        fc_error(fc);
                    }
                    size = atoi(token);
                }
                if (class->type == ct_int) {
                    if (size != 1 && size != 2 && size != 4 && size != 8 && size != 16) {
                        sprintf(fc->sbuf, "Invalid integer byte size, options: 1,2,4,8 or 16 received: '%s'", token);
                        fc_error(fc);
                    }
                    tok_expect(fc, ":", true, false);
                    tok(fc, token, true, false);
                    if (strcmp(token, "true") != 0 && strcmp(token, "false") != 0) {
                        sprintf(fc->sbuf, "Invalid value for is_signed, options: true,false, received: '%s'", token);
                        fc_error(fc);
                    }
                    class->size = size;
                    class->is_signed = strcmp(token, "true") == 0;
                } else {
                    if (size != 4 && size != 8) {
                        sprintf(fc->sbuf, "Invalid float byte size, options: 4 or 8 received: '%s'", token);
                        fc_error(fc);
                    }
                    class->size = size;
                    class->is_signed = true;
                }
            } else {
                sprintf(fc->sbuf, "Unknown class type: '%s'", token);
                fc_error(fc);
            }
        } else if (strcmp(token, "norc") == 0) {
            class->is_rc = false;
        } else if (strcmp(token, "packed") == 0) {
            class->packed = true;
        } else {
            sprintf(fc->sbuf, "Unexpected token: '%s' (class attributes)", token);
            fc_error(fc);
        }

        tok(fc, token, true, true);
    }

    class->chunk_body = chunk_clone(fc->alc, fc->chunk);

    skip_body(fc, '}');
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
