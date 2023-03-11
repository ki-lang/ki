
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