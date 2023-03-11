
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
    }
    if (v->type == v_float) {
    }
    if (v->type == v_ptrv) {
        Value *val = v->item;
        char *lval = llvm_assign_value(b, scope, v);
        return llvm_ir_load(b, val->rett, lval);
    }
    if (v->type == v_op) {
    }
    if (v->type == v_compare) {
    }
    if (v->type == v_fcall) {
    }
    if (v->type == v_fptr) {
    }
    if (v->type == v_class_pa) {
        VClassPA *pa = v->item;
    }
    if (v->type == v_cast) {
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
    }

    die("LLVM : Cannot assign to this value");
    return "?A?";
}