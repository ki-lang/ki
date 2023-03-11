
#include "../all.h"

Value *value_handle_idf(Fc *fc, Allocator *alc, Scope *scope, Id *id, Idf *idf);
void value_equalize_types(Allocator *alc, Fc *fc, Scope *scope, VPair *pair);

Value *value_init(Allocator *alc, int type, void *item, Type *rett) {
    //
    Value *v = al(alc, sizeof(Value));
    v->type = type;
    v->item = item;
    v->rett = rett;

    return v;
}

Value *read_value(Fc *fc, Allocator *alc, Scope *scope, bool sameline, int prio) {
    //
    char *token = fc->token;
    Build *b = fc->b;
    Value *v = NULL;

    tok(fc, token, sameline, true);

    if (strcmp(token, "(") == 0) {
        v = read_value(fc, alc, scope, false, 0);
        //
    } else if (strcmp(token, "\"") == 0) {
        Str *str = read_string(fc);
        char *body = str_to_chars(alc, str);
        v = value_init(alc, v_string, body, type_gen(fc->b, alc, "String"));
        //
    } else if (strcmp(token, "ptrv") == 0) {
        Value *on = read_value(fc, alc, scope, false, 0);
        tok_expect(fc, "as", true, true);
        Type *type = read_type(fc, alc, scope, true, true);
        v = vgen_ptrv(alc, on, type);
        //
    } else if (is_valid_varname_char(token[0])) {
        rtok(fc);
        Id *id = read_id(fc, sameline, true, true);
        Idf *idf = idf_by_id(fc, scope, id, true);
        v = value_handle_idf(fc, alc, scope, id, idf);
    } else {
        sprintf(fc->sbuf, "Unknown value: '%s'", token);
        fc_error(fc);
    }

    if (!v) {
        sprintf(fc->sbuf, "Missing value (bug)");
        fc_error(fc);
    }

    tok(fc, token, true, false);
    while (strcmp(token, ".") == 0 || strcmp(token, "(") == 0 || strcmp(token, "++") == 0 || strcmp(token, "--") == 0) {

        Type *rett = v->rett;

        if (rett->nullable) {
            sprintf(fc->sbuf, "Cannot use '%s' on a nullable type", token);
            fc_error(fc);
        }

        if (strcmp(token, ".") == 0) {

            Class *class = rett->class;
            if (!class) {
                sprintf(fc->sbuf, "Unexpected '.'");
                fc_error(fc);
            }

            tok(fc, token, true, false);

            ClassProp *prop = map_get(class->props, token);
            if (prop) {
                // Class prop
                v = vgen_class_pa(alc, v, prop);
            } else {
                // Class func
                Func *func = map_get(class->funcs, token);
                if (!func) {
                    sprintf(fc->sbuf, "Property does not exist: '%s'", token);
                    fc_error(fc);
                }
                if (func->is_static) {
                    sprintf(fc->sbuf, "Trying to access static function in a non-static way: '%s'", token);
                    fc_error(fc);
                }
                v = vgen_fptr(alc, func, v);
            }

        } else if (strcmp(token, "(") == 0) {
            // // Func call
            // v = read_func_call(fc, scope, v);
        } else if (strcmp(token, "++") == 0 || strcmp(token, "--") == 0) {
            die("TODO: ++ | --");
        }

        tok(fc, token, true, false);
    }

    rtok(fc);
    tok(fc, token, false, true);

    if (prio == 0 || prio > 20) {
        while (strcmp(token, "+") == 0 || strcmp(token, "-") == 0) {
            int op = op_add;
            if (strcmp(token, "-") == 0) {
                op = op_sub;
            }

            Value *right = read_value(fc, alc, scope, false, 20);
            v = value_op(fc, alc, scope, v, right, op);

            tok(fc, token, false, true);
        }
    }

    return v;
}

Value *value_handle_idf(Fc *fc, Allocator *alc, Scope *scope, Id *id, Idf *idf) {
    //
    if (idf->type == idf_var) {
        Var *var = idf->item;
        return value_init(alc, v_var, idf->item, var->type);
    }

    if (idf->type == idf_func) {
        Func *func = idf->item;
    }

    if (idf->type == idf_class) {
        Class *class = idf->item;
    }

    sprintf(fc->sbuf, "Cannot convert identifier to a value: '%s'", id->name);
    fc_error(fc);
}

Value *value_op(Fc *fc, Allocator *alc, Scope *scope, Value *left, Value *right, int op) {

    if (op == op_add) {
        Class *lclass = left->rett->class;
        if (lclass) {
            // let cfunc = lclass.funcs.get("__add") or value null;
            // verify cfunc {
            //     let func = cfunc.func;
            //     let values = Array<Value>.make(2);
            //     values.push(left);
            //     values.push(right);
            //     let on = value_gen_func_ptr(func, null);
            //     fcall_type_check(fc, on, values);
            //     return value_gen_fcall(fc.b, scope, on, values, func.return_type);
            // }
        }
    }

    Type *lt = left->rett;
    Type *rt = right->rett;

    if (lt->type == type_void || rt->type == type_void) {
        sprintf(fc->sbuf, "Cannot use operator on void values");
        fc_error(fc);
    }
    if (lt->type == type_struct || rt->type == type_struct) {
        sprintf(fc->sbuf, "Cannot use operator on class instances");
        fc_error(fc);
    }
    if (lt->type == type_func_ptr || rt->type == type_func_ptr) {
        sprintf(fc->sbuf, "Cannot use operator on function reference values");
        fc_error(fc);
    }
    if (lt->type == type_null || rt->type == type_null) {
        sprintf(fc->sbuf, "Cannot use operator on function reference values");
        fc_error(fc);
    }
    if (lt->nullable || rt->nullable) {
        sprintf(fc->sbuf, "Cannot use operator on null-able values");
        fc_error(fc);
    }

    bool is_ptr = lt->type == type_ptr || rt->type == type_ptr;

    VPair *pair = malloc(sizeof(VPair));
    pair->left = left;
    pair->right = right;

    value_equalize_types(alc, fc, scope, pair);
    Value *l = pair->left;
    Value *r = pair->right;
    free(pair);

    type_check(fc, l->rett, r->rett);
    Value *v = vgen_op(alc, fc->b, l, r, op, is_ptr);

    return v;
}

void value_equalize_types(Allocator *alc, Fc *fc, Scope *scope, VPair *pair) {
    //
}

Value *try_convert(Fc *fc, Allocator *alc, Value *val, Type *to_type) {
    //
}