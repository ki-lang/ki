
#include "../all.h"

Type *type_init(Allocator *alc) {
    //
    Type *type = al(alc, sizeof(Type));
    type->type = type_void;
    type->bytes = 0;
    type->ptr_depth = 0;
    type->is_signed = false;
    type->nullable = false;
    type->class = NULL;
    type->enu = NULL;
    //
    type->func_args = NULL;
    type->func_rett = NULL;
    type->func_errors = NULL;
    type->func_can_error = false;
    //
    return type;
}

int type_get_size(Build *b, Type *type) {
    //
    if (type->bytes > 0) {
        return type->bytes;
    }
    int size = 0;
    if (type->ptr_depth > 0) {
        size = b->ptr_size;
    } else if (type->class) {
        size = type->class->size;
    }
    type->bytes = size;
    return size;
}

bool type_is_void(Type *type) {
    //
    return type->type == type_void;
}

Type *type_gen_class(Allocator *alc, Class *class) {
    //
    int type = type_ptr;
    int bytes = class->fc->b->ptr_size;
    bool is_float = false;
    bool depth = 1;
    bool sign = false;

    if (class->type == ct_struct) {
        type = type_struct;
    } else if (class->type == ct_ptr) {
        type = type_ptr;
    } else if (class->type == ct_int) {
        type = type_int;
        bytes = class->size;
        depth = 0;
        sign = class->is_signed;
    } else if (class->type == ct_float) {
        type = type_float;
        bytes = class->size;
        depth = 0;
        sign = class->is_signed;
    } else {
        die("Compiler bug, unknown class type");
    }

    Type *t = type_init(alc);
    t->type = type;
    t->bytes = bytes;
    t->ptr_depth = depth;
    t->is_signed = sign;
    t->class = class;
    return t;
}

Type *read_type(Fc *fc, Allocator *alc, Scope *scope, bool sameline, bool allow_space) {
    //
    char *token = fc->token;
    bool nullable = false;
    bool t_inline = false;
    bool is_async = false;

    tok(fc, token, sameline, allow_space);

    if (strcmp(token, "?") == 0) {
        nullable = true;
        tok(fc, token, true, false);
    }

    Type *type = type_init(alc);

    if (strcmp(token, "void") == 0) {
        type->type = type_void;
        return type;
    } else if (strcmp(token, "fn") == 0) {
        // Func Ref Type
        type->type = type_func_ref;
        type->bytes = fc->b->ptr_size;
        type->ptr_depth = 1;

        Array *args = array_make(alc, 2);

        tok_expect(fc, "(", true, true);
        tok(fc, token, true, true);
        while (strcmp(token, ")") != 0) {
            rtok(fc);
            Var *arg = var_init(alc, "", read_type(fc, alc, scope, true, true), false, true, false);
            array_push(args, arg);
            tok(fc, token, true, true);
            if (strcmp(token, ",") == 0) {
                tok(fc, token, true, true);
            }
        }
        type->func_args = args;

        // Check return type
        type->func_rett = read_type(fc, alc, scope, true, true);

        // i = tok(fc, token, true, true);

        // if (strcmp(token, "or") == 0) {
        //     // Read error codes
        //     type->func_can_error = true;
        //     fc->i = i;

        //     type->func_error_codes = array_make(2);
        //     while (true) {
        //         fc->i = tok(fc, token, true, true);
        //         if (!is_valid_varname(token)) {
        //             fc_error(fc, "Invalid error code syntax: '%s'", token);
        //         }
        //         if (array_contains(type->func_error_codes, token, "chars")) {
        //             fc_error(fc, "Duplicate error code: '%s'", token);
        //         }

        //         array_push(type->func_error_codes, strdup(token));

        //         i = tok(fc, token, true, true);
        //         if (strcmp(token, ",") == 0) {
        //             fc->i = i;
        //             continue;
        //         }
        //         break;
        //     }
        // }
    } else {

        rtok(fc);

        Id *id = read_id(fc, sameline, allow_space, true);
        Idf *idf = idf_by_id(fc, scope, id, true);

        if (!idf) {
            sprintf(fc->sbuf, "Unknown type: '%s'", token);
            fc_error(fc);
        } else {
            if (idf->type == idf_class) {
                Class *class = idf->item;
                // Check generic
                // if (class->is_generic_base) {
                //     //
                //     Array *generic_types = read_generic_types(fc, scope, class);
                //     char hash[33];
                //     class_generate_generic_hash(generic_types, hash);
                //     class = class_get_generic_class(class, hash, generic_types, class->fc != fc);
                // }
                type = type_gen_class(alc, class);
                //
            } else if (idf->type == idf_enum) {
                type = type_gen(fc->b, alc, "i32");
                type->enu = idf->item;
            } else if (idf->type == idf_fc) {
                Fc *fc_ = idf->item;
                tok_expect(fc, ".", true, false);
                type = read_type(fc, alc, fc_->scope, true, false);
            } else {
                sprintf(fc->sbuf, "Not a type");
                fc_error(fc);
            }
        }
    }

    //
    if (nullable) {
        if (type->ptr_depth == 0) {
            sprintf(fc->sbuf, "Only pointer types can be nullable.");
            fc_error(fc);
        }
        type->nullable = nullable;
    }

    if (type->bytes == 0) {
        array_push(fc->type_size_checks, type);
    }

    return type;
}

Type *type_gen(Build *b, Allocator *alc, char *name) {

    Pkc *pkc = b->pkc_ki;
    Nsc *nsc = pkc_get_nsc(pkc, "type");

    Idf *idf = map_get(nsc->scope->identifiers, name);
    if (!idf || idf->type != idf_class) {
        sprintf(b->sbuf, "Type not found '%s'", name);
        die(b->sbuf);
    }

    return type_gen_class(alc, idf->item);
}
