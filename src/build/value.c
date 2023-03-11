
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
        tok_expect(fc, ")", false, true);
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
    } else if (strcmp(token, "sizeof") == 0) {
        tok_expect(fc, "(", true, false);
        Type *type = read_type(fc, alc, scope, false, true);
        tok_expect(fc, ")", false, true);
        v = vgen_vint(alc, type->bytes, type_gen(b, alc, "u32"), false);
        //
    } else if (is_number(token[0]) || strcmp(token, "-") == 0) {

        bool is_negative = strcmp(token, "-") == 0;

        if (is_negative) {
            tok(fc, token, true, false);
        }

        bool is_float = false;
        long int iv = 0;
        float fv = 0;

        if (strcmp(token, "0") == 0 && get_char(fc, 0) == 'x') {
            //
            die("TODO: hex numbers");
        } else if (is_number(token[0])) {
            char *num_str = dups(alc, token);
            char *float_str = NULL;
            if (get_char(fc, 0) == '.') {

                if (is_number(get_char(fc, 1))) {
                    is_float = true;

                    chunk_move(fc->chunk, 1);
                    tok(fc, token, true, false);

                    float_str = al(alc, strlen(num_str) + strlen(token) + 2);
                    strcpy(float_str, num_str);
                    strcpy(float_str, ".");
                    strcpy(float_str, token);
                }
            }
            if (is_float) {
                fv = atof(float_str);
            } else {
                iv = atoi(num_str);
            }
        } else {
            sprintf(fc->sbuf, "Invalid number: '%s'", token);
            fc_error(fc);
        }

        if (is_float) {
            if (is_negative) {
                fv *= -1;
            }
            v = vgen_vfloat(alc, fc->b, fv, false);
        } else {
            if (is_negative) {
                iv *= -1;
            }
            if (get_char(fc, 0) == '#') {
                chunk_move(fc->chunk, 1);
                Type *type = read_type(fc, alc, scope, true, false);
                if (type->type != type_int) {
                    sprintf(fc->sbuf, "Invalid integer type");
                    fc_error(fc);
                }
                v = vgen_vint(alc, iv, type, true);
            } else {
                v = vgen_vint(alc, iv, type_gen(fc->b, alc, "i32"), false);
            }
        }

        //
    } else if (is_valid_varname_char(token[0])) {
        rtok(fc);
        Id *id = read_id(fc, sameline, true, true);
        Idf *idf = idf_by_id(fc, scope, id, true);
        v = value_handle_idf(fc, alc, scope, id, idf);
    } else {
        sprintf(fc->sbuf, "Unknown value: '%s' | %d", token, prio);
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

    if (prio == 0 || prio > 30) {
        sprintf(fc->sbuf, ".%s.", token);
        while (strstr(".==.!=.<=.>=.<.>.", fc->sbuf)) {
            int op = op_eq;
            if (strcmp(token, "!=") == 0) {
                op = op_ne;
            } else if (strcmp(token, "<") == 0) {
                op = op_lt;
            } else if (strcmp(token, "<=") == 0) {
                op = op_lte;
            } else if (strcmp(token, ">") == 0) {
                op = op_gt;
            } else if (strcmp(token, ">=") == 0) {
                op = op_gte;
            }

            Value *right = read_value(fc, alc, scope, false, 30);

            bool magic = false;

            if (op == op_eq || op == op_ne) {
                Class *lclass = v->rett->class;
                // if(lclass){
                //     let cfunc = lclass.funcs.get("__eq") or value null;
                //     verify cfunc {
                //         let func = cfunc.func;
                //         let values = Array<Value>.make(2);
                //         values.push(v);
                //         values.push(right);
                //         let on = value_gen_func_ptr(func, null);
                //         fcall_type_check(fc, on, values);
                //         v = value_gen_fcall(fc.b, scope, on, values, func.return_type);
                //         magic = true;
                //     }
                // }
            }

            if (!magic) {
                VPair *pair = malloc(sizeof(VPair));
                pair->left = v;
                pair->right = right;
                value_equalize_types(alc, fc, scope, pair);
                Value *left = pair->left;
                Value *right = pair->right;
                type_check(fc, left->rett, right->rett);
                v = vgen_compare(alc, b, left, right, op);
            }

            tok(fc, token, false, true);
            sprintf(fc->sbuf, ".%s.", token);
        }
    }

    rtok(fc);

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
    Build *b = fc->b;
    Value *left = pair->left;
    Value *right = pair->right;
    Type *lt = left->rett;
    Type *rt = right->rett;

    // structs -> ptr
    // ptr -> uxx
    // numbers to biggest size

    if (lt->type == type_struct) {
        left = vgen_cast(alc, left, type_gen(b, alc, "ptr"));
        pair->left = left;
        lt = left->rett;
    }
    if (rt->type == type_struct) {
        right = vgen_cast(alc, right, type_gen(b, alc, "ptr"));
        pair->right = right;
        rt = right->rett;
    }
    if (lt->type == type_ptr) {
        left = vgen_cast(alc, left, type_gen(b, alc, "uxx"));
        pair->left = left;
        lt = left->rett;
    }
    if (rt->type == type_ptr) {
        right = vgen_cast(alc, right, type_gen(b, alc, "uxx"));
        pair->right = right;
        rt = right->rett;
    }
    if (lt->type == type_null) {
        left = vgen_vint(alc, 0, type_gen(b, alc, "u8"), false);
        pair->left = left;
        lt = left->rett;
    }
    if (rt->type == type_null) {
        right = vgen_vint(alc, 0, type_gen(b, alc, "u8"), false);
        pair->right = right;
        rt = right->rett;
    }

    if (lt->type != type_int || rt->type != type_int) {
        die("Could not convert value to a number\n");
        return;
    }

    if (left->type == v_vint && right->type == v_vint) {
        VInt *lint = left->item;
        VInt *rint = right->item;
        if (lint->force_type == false && rint->force_type == false) {
            // If both are number literals, dont convert them
            return;
        }
    }

    if (left->type == v_vint) {
        VInt *vint = left->item;
        if (vint->force_type == false) {
            left = try_convert(fc, alc, left, right->rett);
            pair->left = left;
            lt = left->rett;
            return;
        }
    }

    if (right->type == v_vint) {
        VInt *vint = right->item;
        if (vint->force_type == false) {
            right = try_convert(fc, alc, right, left->rett);
            pair->right = right;
            rt = right->rett;
            return;
        }
    }

    int bytes = lt->bytes;
    bool is_signed = lt->is_signed || rt->is_signed;
    if (rt->bytes > bytes) {
        bytes = rt->bytes;
    }

    if (lt->bytes < bytes) {
        left = vgen_cast(alc, left, type_gen_int(b, alc, bytes, is_signed));
        pair->left = left;
        lt = left->rett;
    }
    if (rt->bytes < bytes) {
        right = vgen_cast(alc, right, type_gen_int(b, alc, bytes, is_signed));
        pair->right = right;
        rt = right->rett;
    }
}

Value *try_convert(Fc *fc, Allocator *alc, Value *val, Type *to_type) {
    //
    // Class* str_class = fc.b.get_ki_class("String");
    // if totype
    //     .class == str_class {
    //         let this_class = this.rett.class;
    //         verify this_class {
    //             if this_class
    //                 != str_class {
    //                     let cfunc = this_class.funcs.get("__string") or value null;
    //                     verify cfunc {
    //                         let func = cfunc.func;
    //                         let values = Array<Value>.make(2);
    //                         values.push(this);
    //                         let on = value_gen_func_ptr(func, null);
    //                         fcall_type_check(fc, on, values);
    //                         return value_gen_fcall(fc.b, scope, on, values, func.return_type);
    //                     }
    //                 }
    //         }
    //     }

    Type *vt = val->rett;
    if (val->type == v_vint) {
        VInt *vint = val->item;
        if (to_type->type == type_int || to_type->type == type_float) {
            // vint -> vint|vfloat
            if (!vint->force_type && (vt->bytes != to_type->bytes || vt->is_signed != to_type->is_signed)) {
                if (to_type->type == type_float) {
                    return vgen_vfloat(alc, fc->b, (float)vint->value, false);
                }
                int bytes = to_type->bytes;
                int bits = bytes * 8;
                // Doesnt work for some reason
                // if bytes >= fc.b.ptr_size {
                //    bits = fc.b.ptr_size * 8 - 1;
                //}
                if (bytes >= 4) {
                    bits = 4 * 8 - 1;
                }
                long int max = ((long int)1) << bits;
                long int min = 0;
                if (to_type->is_signed) {
                    min = max * -1;
                }
                if (vint->value >= min && vint->value <= max) {
                    val->rett = to_type;
                    // } else {
                    //     print(f "{} || {} - {} ({})\n" {vint.value.str(), min.str(), max.str(), bits});
                }
            }
        } else if (type_is_ptr(to_type, fc->b)) {
            // vint -> ptr
            return vgen_cast(alc, val, to_type);
        }
        return val;
    }

    if (vt->type == type_int || vt->type == type_float) {
        if (to_type->type == type_int || to_type->type == type_float) {
            if (to_type->type == type_float) {
                // Float -> {int/float}
                // TODO
            } else {
                // Int -> {int/float}
                if (vt->bytes < to_type->bytes) {
                    return vgen_cast(alc, val, to_type);
                }
            }
        }
        if (type_is_ptr(to_type, fc->b))
            return vgen_cast(alc, val, to_type);

        return val;
    }

    if (vt->type == type_null) {
        if (to_type->nullable) {
            return vgen_cast(alc, val, to_type);
        }
    }

    return val;
}