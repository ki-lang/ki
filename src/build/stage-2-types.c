
#include "../all.h"

void stage_2_class(Fc *fc, Class *class);
void stage_2_class_props(Fc *fc, Class *class);
void stage_2_func(Fc *fc, Func *func);

void stage_2(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 0) {
        printf("# Stage 2 : %s\n", fc->path_ki);
    }

    for (int i = 0; i < fc->classes->length; i++) {
        Class *class = array_get_index(fc->classes, i);
        if (b->verbose > 1) {
            printf("> Scan class properties: %s\n", class->dname);
        }
        stage_2_class(fc, class);
    }
    for (int i = 0; i < fc->funcs->length; i++) {
        Func *func = array_get_index(fc->funcs, i);
        if (b->verbose > 1) {
            printf("> Scan func types: %s\n", func->dname);
        }
        stage_2_func(fc, func);
    }

    //
    chain_add(b->stage_3, fc);
}

void stage_2_class(Fc *fc, Class *class) {
    //
    if (class->is_generic_base) {
        return;
    }
    //

    //
    fc->chunk = class->chunk_body;
    stage_2_class_props(fc, class);
}

void stage_2_class_props(Fc *fc, Class *class) {
    //
    char *token = fc->token;
    Scope *scope = class->scope;

    while (true) {

        tok(fc, token, false, true);

        if (token[0] == 0) {
            sprintf(fc->sbuf, "Unexpected end of file");
            fc_error(fc);
        }

        if (strcmp(token, "}") == 0) {
            break;
        }

        if (strcmp(token, "/") == 0 && get_char(fc, 0) == '/') {
            skip_body(fc, '\n');
            continue;
        }

        if (strcmp(token, "#") == 0) {
            read_macro(fc, fc->alc, scope);
            continue;
        }

        bool is_static = false;

        if (strcmp(token, "static") == 0) {
            is_static = true;
            tok(fc, token, true, true);
        }

        if (strcmp(token, "func") == 0) {
            // Function
            tok(fc, token, true, true);
            if (!is_valid_varname(token)) {
                sprintf(fc->sbuf, "Invalid function name syntax: '%s'", token);
                fc_error(fc);
            }
            if (map_get(class->props, token) || map_get(class->funcs, token)) {
                sprintf(fc->sbuf, "Name already used (func name): '%s'", token);
                fc_error(fc);
            }

            char *name = dups(fc->alc, token);
            char *gname = nsc_gname(fc->nsc, name);
            char *dname = nsc_dname(fc->nsc, name);

            Func *func = func_init(fc->alc);
            func->fc = fc;
            func->name = name;
            func->gname = gname;
            func->dname = dname;
            func->scope = scope_init(fc->alc, sct_func, fc->scope);
            func->scope->func = func;
            func->is_static = is_static;

            array_push(fc->funcs, func);
            map_set(class->funcs, name, func);

            tok_expect(fc, "(", true, true);

            func->chunk_args = chunk_clone(fc->alc, fc->chunk);

            // First arg
            if (!is_static) {
                array_push(func->args, var_init(fc->alc, "this", type_gen_class(fc->alc, class), false, true, false));
            }

            skip_body(fc, ')');
            skip_until_char(fc, '{');
            func->chunk_body = chunk_clone(fc->alc, fc->chunk);
            skip_body(fc, '}');

        } else {
            // Property
            if (is_static) {
                sprintf(fc->sbuf, "Static properties are not allowed, use globals instead");
                fc_error(fc);
            }

            if (!is_valid_varname(token)) {
                sprintf(fc->sbuf, "Invalid function name syntax: '%s'", token);
                fc_error(fc);
            }
            if (map_get(class->props, token) || map_get(class->funcs, token)) {
                sprintf(fc->sbuf, "Name already used (property name): '%s'", token);
                fc_error(fc);
            }

            ClassProp *prop = class_prop_init(fc->alc);
            prop->index = class->props->values->length;

            tok(fc, token, true, true);
            if (strcmp(token, ":") == 0) {
                Type *type = read_type(fc, fc->alc, scope, true, true);
                if (type_is_void(type)) {
                    sprintf(fc->sbuf, "Cannot use 'void' as a type for a class property");
                    fc_error(fc);
                }
                prop->type = type;

                tok(fc, token, true, true);
            }

            if (strcmp(token, "=") == 0) {
                prop->value_chunk = chunk_clone(fc->alc, fc->chunk);
            } else {
                rtok(fc);
            }

            if (!prop->type && !prop->value_chunk) {
                sprintf(fc->sbuf, "The property must either have a type or default value defined. Both are missing.");
                fc_error(fc);
            }

            map_set(class->props, token, prop);

            skip_until_char(fc, ';');
        }
    }
}

void stage_2_func(Fc *fc, Func *func) {
    //
}