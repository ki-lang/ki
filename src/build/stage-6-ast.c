
#include "../all.h"

void stage_6_func(Fc *fc, Func *func);

void stage_6(Fc *fc) {
    //
    if (fc->is_header)
        return;

    Build *b = fc->b;
    if (b->verbose > 1) {
        printf("# Stage 6 : AST : %s\n", fc->path_ki);
    }

    for (int i = 0; i < fc->funcs->length; i++) {
        Func *func = array_get_index(fc->funcs, i);
        if (b->verbose > 1) {
            printf("> Read func AST: %s\n", func->dname);
        }
        stage_6_func(fc, func);
    }

    // Write IR
}

void stage_6_func(Fc *fc, Func *func) {
    //

    if (func->is_generated) {
        return;
    }

    Chunk *chunk = func->chunk_body;
    fc->chunk = chunk;

    read_ast(fc, func->scope, false);
}

void read_ast(Fc *fc, Scope *scope, bool single_line) {
    //
    char *token = fc->token;
    bool first_line = true;
    Allocator *alc = fc->alc_ast;

    while (true) {
        //
        tok(fc, token, false, true);

        if (token[0] == 0) {
            sprintf(fc->sbuf, "Unexpected end of file");
            fc_error(fc);
        }

        if (strcmp(token, "#") == 0) {
            read_macro(fc, alc, scope);
            continue;
        }
        first_line = false;

        if (strcmp(token, "}") == 0 && !single_line) {
            break;
        }

        // if (strcmp(token, "let") == 0) {
        //     continue;
        // }

        //
        rtok(fc);
        Value *val = read_value(fc, alc, scope, false, 0);

        // Statement
        if (type_is_void(val->rett)) {
            sprintf(fc->sbuf, "Statement returns a value, but no variable to store it in");
            fc_error(fc);
        }

        array_push(scope->ast, token_init(alc, tkn_statement, val));
        tok_expect(fc, ";", false, true);
    }
}