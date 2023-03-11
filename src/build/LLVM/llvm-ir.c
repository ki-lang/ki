
#include "../../headers/LLVM.h"

void llvm_ir_jump(Str *ir, LLVMBlock *block) {
    str_append_chars(ir, "  br label %");
    str_append_chars(ir, block->name);
    str_append_chars(ir, "\n");
}

void llvm_ir_cond_jump(LB *b, Str *ir, char *var_i1, LLVMBlock *a_block, LLVMBlock *b_block) {
    str_append_chars(ir, "  br i1 ");
    str_append_chars(ir, var_i1);
    str_append_chars(ir, ", label %");
    str_append_chars(ir, a_block->name);
    str_append_chars(ir, ", label %");
    str_append_chars(ir, b_block->name);
    str_append_chars(ir, "\n");
}

void llvm_ir_store(LB *b, Type *type, char *var, char *val) {
    Str *ir = llvm_b_ir(b);
    char *ltype = llvm_type(b, type);

    char bytes[20];
    sprintf(bytes, "%d", type->bytes);

    str_append_chars(ir, "  store ");
    str_append_chars(ir, ltype);
    str_append_chars(ir, " ");
    str_append_chars(ir, val);
    str_append_chars(ir, ", ");
    str_append_chars(ir, ltype);
    str_append_chars(ir, "* ");
    str_append_chars(ir, var);
    str_append_chars(ir, ", align ");
    str_append_chars(ir, bytes);
    str_append_chars(ir, "\n");
}
char *llvm_ir_load(LB *b, Type *type, char *var) {

    Str *ir = llvm_b_ir(b);
    char *var_result = llvm_var(b);
    char *ltype = llvm_type(b, type);

    char bytes[20];
    sprintf(bytes, "%d", type->bytes);

    str_append_chars(ir, "  ");
    str_append_chars(ir, var_result);
    str_append_chars(ir, " = load ");
    str_append_chars(ir, ltype);
    str_append_chars(ir, ", ");
    str_append_chars(ir, ltype);
    str_append_chars(ir, "* ");
    str_append_chars(ir, var);
    str_append_chars(ir, ", align ");
    str_append_chars(ir, bytes);
    str_append_chars(ir, "\n");

    return var_result;
}

char *llvm_ir_bool_i1(LB *b, Str *ir, char *val) {
    char *var_i1 = llvm_var(b);
    str_append_chars(ir, "  ");
    str_append_chars(ir, var_i1);
    str_append_chars(ir, " = trunc i8 ");
    str_append_chars(ir, val);
    str_append_chars(ir, " to i1\n");
    return var_i1;
}

char *llvm_ir_class_prop_access(LB *b, Class *class, char *on, ClassProp *prop) {
    char *result = llvm_var(b);
    Str *ir = llvm_b_ir(b);

    char index[20];
    sprintf(index, "%d", prop->index);

    str_append_chars(ir, "  ");
    str_append_chars(ir, result);
    str_append_chars(ir, " = getelementptr inbounds %struct.");
    str_append_chars(ir, class->gname);
    str_append_chars(ir, ", %struct.");
    str_append_chars(ir, class->gname);
    str_append_chars(ir, "* ");
    str_append_chars(ir, on);
    str_append_chars(ir, ", i32 0, i32 ");
    str_append_chars(ir, index);
    str_append_chars(ir, "\n");

    return result;
}

Array *llvm_ir_fcall_args(LB *b, Scope *scope, Array *values) {
    Array *result = array_make(b->alc, values->length + 1);
    for (int i = 0; i < values->length; i++) {
        Value *val = array_get_index(values, i);
        char *lval = llvm_value(b, scope, val);
        char *buf = b->fc->sbuf;
        sprintf(buf, "%s noundef %s", llvm_type(b, val->rett), lval);
        array_push(result, dups(b->alc, lval));
    }
    return result;
}

char *llvm_ir_func_call(LB *b, char *on, Array *values, char *lrett, bool can_error) {
    Str *ir = llvm_b_ir(b);
    if (can_error) {
        die("TODO LLVM Func error");
        // TODO dont reset err before each call, reset when err is caught instead
        // llvm_ir_store(b, type_gen(b->fc->b, "i32"), b->func_buf_err, "0");
    }

    char *var = "";
    str_append_chars(ir, "  ");
    if (strcmp(lrett, "void") != 0) {
        var = llvm_var(b);
        str_append_chars(ir, var);
        str_append_chars(ir, " = ");
    }
    str_append_chars(ir, "call ");
    str_append_chars(ir, lrett);
    str_append_chars(ir, " ");
    str_append_chars(ir, on);
    str_append_chars(ir, "(");
    int argc = values->length;
    for (int i = 0; i < values->length; i++) {
        char *lval = array_get_index(values, i);
        if (i > 0) {
            str_append_chars(ir, ", ");
        }
        str_append_chars(ir, lval);
    }
    if (can_error) {
        die("TODO LLVM Func error");
        // if (argc > 0) {
        //     str_append_chars(ir, ", ");
        // }
        // str_append_chars(ir, "i32* noundef ");
        // str_append_chars(ir, b->func_buf_err);
        // str_append_chars(ir, ", i8** noundef ");
        // str_append_chars(ir, b->func_buf_msg);
    }
    str_append_chars(ir, ")\n");
    return var;
}