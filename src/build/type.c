
#include "../all.h"

Type *type_init(Allocator *alc) {
    //
    Type *type = al(alc, sizeof(Type));
    type->type = type_void;
    type->bytes = 0;
    type->ptr_depth = 0;
    type->is_signed = false;
    type->nullable = false;
    type->owned = false;
    type->shared = false;
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
bool type_is_ptr(Type *type, Build *b) {
    //
    return type->class == ki_get_class(b, "type", "ptr");
}
bool type_is_bool(Type *type, Build *b) {
    //
    return type->class == ki_get_class(b, "type", "bool");
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

Type *type_gen_fptr(Allocator *alc, Func *func) {
    //
    Type *t = type_init(alc);
    t->type = type_func_ptr;
    t->bytes = func->fc->b->ptr_size;
    t->ptr_depth = 1;

    t->func_args = func->args;
    t->func_rett = func->rett;
    // t->func_errors = func->errors;
    // t->func_can_error = func->can_error;

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
        type->type = type_func_ptr;
        type->bytes = fc->b->ptr_size;
        type->ptr_depth = 1;

        Array *args = array_make(alc, 2);

        tok_expect(fc, "(", true, true);
        tok(fc, token, true, true);
        while (strcmp(token, ")") != 0) {
            rtok(fc);
            Arg *arg = arg_init(alc, "", read_type(fc, alc, scope, true, true), false);
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

Type *type_gen_int(Build *b, Allocator *alc, int bytes, bool is_signed) {
    if (bytes == 1) {
        if (is_signed) {
            return type_gen(b, alc, "i8");
        } else {
            return type_gen(b, alc, "u8");
        }
    } else if (bytes == 2) {
        if (is_signed) {
            return type_gen(b, alc, "i16");
        } else {
            return type_gen(b, alc, "u16");
        }
    } else if (bytes == 4) {
        if (is_signed) {
            return type_gen(b, alc, "i32");
        } else {
            return type_gen(b, alc, "u32");
        }
    } else if (bytes == 8) {
        if (is_signed) {
            return type_gen(b, alc, "i64");
        } else {
            return type_gen(b, alc, "u64");
        }
    }
    die("Cannot generate integer type (bug)");
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

bool type_compat(Type *t1, Type *t2, char **reason) {
    //
    bool t1void = type_is_void(t1);
    bool t2void = type_is_void(t2);
    if (t1void || t2void) {
        return t1void == t2void;
    }
    int t1t = t1->type;
    int t2t = t2->type;
    if (t2t == type_null && t1->nullable) {
        return true;
    }
    if (t1t != t2t) {
        if (reason)
            *reason = "Types are not compatible";
        return false;
    }
    if (t1->bytes != t2->bytes) {
        if (reason)
            *reason = "The types have different sizes";
        return false;
    }
    if (t1->ptr_depth != t2->ptr_depth) {
        if (reason)
            *reason = "Pointer depth difference";
        return false;
    }
    if (t1t == type_int) {
        if (t1->enu != NULL && t1->enu != t2->enu) {
            if (reason)
                *reason = "Right side number must be from the same enum type";
            return false;
        }
        if (t2->is_signed != t1->is_signed) {
            if (reason)
                *reason = "Signed & unsigned are not compatible, use cast";
            return false;
        }
    } else if (t1t == type_struct) {
        if (t1->class != t2->class) {
            if (reason)
                *reason = "Classes are not the same";
            return false;
        }
    }
    if (t2->nullable && !t1->nullable) {
        if (reason)
            *reason = "Trying to assign nullable type to a non nullable type";
        return false;
    }
    if (t1t == type_func_ptr) {
        die("TODO: type check func ptr");
        //     Array *t1_args = t1->func_args;
        //     Array *t2_args = t2->func_args;
        //     if (t1_args->length != t2_args->length) {
        //         return false;
        //     }
        //     for (int i = 0; i < t1_args->length; i++) {
        //         VarDecl *t1d = array_get_index(t1_args, i);
        //         VarDecl *t2d = array_get_index(t2_args, i);
        //         if (!type_compatible(t1d->type, t2d->type)) {
        //             return false;
        //         }
        //     }
        //     if (!type_compatible(t1->func_return_type, t2->func_return_type)) {
        //         return false;
        //     }
        //     // Check error codes
        //     Array *t1errs = t1->func_error_codes;
        //     Array *t2errs = t2->func_error_codes;
        //     if ((t1errs == NULL || t2errs == NULL) && t1errs != t2errs) {
        //         return false;
        //     }
        //     if (t1->func_error_codes) {
        //         if (t1errs->length != t2errs->length) {
        //             return false;
        //         }
        //         for (int i = 0; i < t1errs->length; i++) {
        //             char *t1e = array_get_index(t1errs, i);
        //             char *t2e = array_get_index(t2errs, i);
        //             if (strcmp(t1e, t2e) != 0) {
        //                 return false;
        //             }
        //         }
        //     }
    }
    return true;
}

char *type_to_str(Type *t, char *res) {
    //
    strcpy(res, "");
    if (t->type == type_ptr && t->class == NULL) {
        strcat(res, "ptr");
        return res;
    }
    if (t->nullable) {
        strcat(res, "?");
    }
    if (t->ptr_depth - 1 > 0) {
        strcat(res, "*");
    }
    if (t->class) {
        Class *class = t->class;
        strcat(res, class->dname);
    } else if (t->type == type_null) {
        strcat(res, "null");
    } else if (t->type == type_func_ptr) {
        strcat(res, "fn (");
        char sub_str[256];
        for (int i = 0; i < t->func_args->length; i++) {
            if (i > 0) {
                strcat(res, ", ");
            }
            Arg *arg = array_get_index(t->func_args, i);
            strcat(res, type_to_str(arg->type, sub_str));
        }
        strcat(res, ") ");
        strcat(res, type_to_str(t->func_rett, sub_str));
        if (t->func_errors) {
            strcat(res, " or ");
            for (int i = 0; i < t->func_errors->length; i++) {
                if (i > 0) {
                    strcat(res, ", ");
                }
                strcat(res, array_get_index(t->func_errors, i));
            }
        }
    } else {
        strcat(res, "(Unknown type)");
    }
    return res;
}

void type_check(Fc *fc, Type *t1, Type *t2) {
    //
    char *reason;
    if (!type_compat(t1, t2, &reason)) {
        char t1s[200];
        char t2s[200];
        type_to_str(t1, t1s);
        type_to_str(t2, t2s);
        sprintf(fc->sbuf, "Types are not compatible: %s <-- %s \nReason: %s", t1s, t2s, reason);
        fc_error(fc);
    }
}
