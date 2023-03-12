
#include "../../headers/LLVM.h"

char *llvm_value(LB *b, Scope *scope, Value *v) {
    //
    if (v->type == v_var) {
        Var *var = v->item;
        char *var_val = llvm_get_var(b, scope, var);
        if (!var->is_mut) {
            return var_val;
        }
        return llvm_ir_load(b, var->type, var_val);
    }
    if (v->type == v_vint) {
        VInt *vint = v->item;
        char *res = al(b->alc, 20);
        sprintf(res, "%ld", vint->value);
        return res;
    }
    if (v->type == v_float) {
        VFloat *vfl = v->item;
        char *res = al(b->alc, 20);
        sprintf(res, "%f", vfl->value);
        return res;
    }
    if (v->type == v_string) {
        return llvm_ir_string(b, v->item);
    }
    if (v->type == v_ptrv) {
        char *lval = llvm_assign_value(b, scope, v);
        return llvm_ir_load(b, v->rett, lval);
    }
    if (v->type == v_op) {
        VOp *vop = v->item;
        int op = vop->op;
        char *lval1 = llvm_value(b, scope, vop->left);
        char *lval2 = llvm_value(b, scope, vop->right);
        Type *type = v->rett;
        char *ltype = llvm_type(b, type);
        char *var = llvm_var(b);

        Str *ir = llvm_b_ir(b);
        str_append_chars(ir, "  ");
        str_append_chars(ir, var);
        str_append_chars(ir, " = ");
        if (op == op_add) {
            str_append_chars(ir, "add ");
        } else if (op == op_sub) {
            str_append_chars(ir, "sub ");
        } else if (op == op_mul) {
            str_append_chars(ir, "mul ");
        } else if (op == op_div) {
            if (type->is_signed) {
                str_append_chars(ir, "sdiv ");
            } else {
                str_append_chars(ir, "udiv ");
            }
        } else if (op == op_mod) {
            if (type->is_signed) {
                str_append_chars(ir, "srem ");
            } else {
                str_append_chars(ir, "urem ");
            }
        } else if (op == op_bit_and) {
            str_append_chars(ir, "and ");
        } else if (op == op_bit_or) {
            str_append_chars(ir, "or ");
        } else if (op == op_bit_xor) {
            str_append_chars(ir, "xor ");
        } else if (op == op_shl) {
            str_append_chars(ir, "shl ");
        } else if (op == op_shr) {
            str_append_chars(ir, "shr ");
        } else {
            die("Unknown LLVM math operation (compiler bug)");
        }
        str_append_chars(ir, ltype);
        str_append_chars(ir, " ");
        str_append_chars(ir, lval1);
        str_append_chars(ir, ", ");
        str_append_chars(ir, lval2);
        str_append_chars(ir, "\n");

        return var;
    }
    if (v->type == v_compare) {
        VOp *vop = v->item;
        int op = vop->op;
        char *lval1 = llvm_value(b, scope, vop->left);
        char *lval2 = llvm_value(b, scope, vop->right);
        Type *type = v->rett;
        char *ltype = llvm_type(b, type);

        bool is_signed = type->is_signed;
        bool is_float = type->type == type_float;

        char *var_tmp = llvm_var(b);
        char *var_result = llvm_var(b);

        char *sign = "eq";
        if (op == op_ne) {
            sign = "ne";
        } else if (op == op_lt) {
            if (is_float) {
                sign = "olt";
            } else if (is_signed) {
                sign = "slt";
            } else {
                sign = "ult";
            }
        } else if (op == op_lte) {
            if (is_float) {
                sign = "ole";
            } else if (is_signed) {
                sign = "sle";
            } else {
                sign = "ule";
            }
        } else if (op == op_gt) {
            if (is_float) {
                sign = "ogt";
            } else if (is_signed) {
                sign = "sgt";
            } else {
                sign = "ugt";
            }
        } else if (op == op_gte) {
            if (is_float) {
                sign = "oge";
            } else if (is_signed) {
                sign = "sge";
            } else {
                sign = "uge";
            }
        }

        Str *ir = llvm_b_ir(b);
        str_append_chars(ir, "  ");
        str_append_chars(ir, var_tmp);
        str_append_chars(ir, " = icmp ");
        str_append_chars(ir, sign);
        str_append_chars(ir, " ");
        str_append_chars(ir, ltype);
        str_append_chars(ir, " ");
        str_append_chars(ir, lval1);
        str_append_chars(ir, ", ");
        str_append_chars(ir, lval2);
        str_append_chars(ir, "\n");

        str_append_chars(ir, "  ");
        str_append_chars(ir, var_result);
        str_append_chars(ir, " = zext i1 ");
        str_append_chars(ir, var_tmp);
        str_append_chars(ir, " to i8\n");

        return var_result;
    }
    if (v->type == v_fcall) {
        VFcall *fcall = v->item;
        char *on = llvm_value(b, scope, fcall->on);
        Array *values = llvm_ir_fcall_args(b, scope, fcall->args);
        return llvm_ir_func_call(b, on, values, llvm_type(b, v->rett), fcall->on->rett->func_can_error);
    }
    if (v->type == v_fptr) {
        VFuncPtr *fptr = v->item;
        return llvm_ir_func_ptr(b, fptr->func);
    }
    if (v->type == v_class_pa) {
        VClassPA *pa = v->item;
        char *lval = llvm_assign_value(b, scope, v);
        return llvm_ir_load(b, pa->prop->type, lval);
    }
    if (v->type == v_cast) {
        Value *val = v->item;
        Type *from_type = val->rett;
        Type *to_type = v->rett;
        char *lval = llvm_value(b, scope, val);
        return llvm_ir_cast(b, lval, from_type, to_type);
    }
    return "???";
}

char *llvm_assign_value(LB *b, Scope *scope, Value *v) {
    //
    if (v->type == v_var) {
        Var *var = v->item;
        return llvm_get_var(b, scope, var);
    }
    if (v->type == v_ptrv) {
        Value *val = v->item;
        Type *as_type = v->rett;

        char *lval = llvm_value(b, scope, val);
        char *ltype = llvm_type(b, as_type);
        char *var_result = llvm_var(b);

        Str *ir = llvm_b_ir(b);
        str_append_chars(ir, "  ");
        str_append_chars(ir, var_result);
        str_append_chars(ir, " = bitcast i8* ");
        str_append_chars(ir, lval);
        str_append_chars(ir, " to ");
        str_append_chars(ir, ltype);
        str_append_chars(ir, "*\n");

        return var_result;
    }
    if (v->type == v_class_pa) {
        VClassPA *pa = v->item;
        Value *on = pa->on;
        ClassProp *prop = pa->prop;
        Class *class = on->rett->class;
        Type *type = prop->type;
        char *lon = llvm_value(b, scope, on);
        return llvm_ir_class_prop_access(b, class, lon, prop);
    }

    die("LLVM : Cannot assign to this value");
    return "?A?";
}