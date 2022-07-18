
#include "../all.h"

void llvm_build_if_token(FileCompiler *fc, TokenIf *ift, LLVMBasicBlockRef after) {
    // Create blocks
    LLVMBasicBlockRef ifcode = LLVMAppendBasicBlock(fc->current_func, llvm_buf(fc));
    LLVMBasicBlockRef elsecode = ift->next ? LLVMAppendBasicBlock(fc->current_func, llvm_buf(fc)) : NULL;
    // Cond check
    LLVMValueRef cond = llvm_value(fc, ift->condition);
    LLVMValueRef ifv = LLVMBuildICmp(fc->builder, LLVMIntNE, cond, llvm_u8(0), llvm_buf(fc));
    LLVMBuildCondBr(fc->builder, ifv, ifcode, elsecode ? elsecode : after);
    // If code
    LLVMPositionBuilderAtEnd(fc->builder, ifcode);
    llvm_build_ast(fc, ift->scope);
    LLVMBuildBr(fc->builder, after);
    // Else code
    if (ift->next) {
        LLVMPositionBuilderAtEnd(fc->builder, elsecode);
        llvm_build_if_token(fc, ift->next, after);
        LLVMBuildBr(fc->builder, after);
    }
    // Result
    LLVMPositionBuilderAtEnd(fc->builder, after);
}

void llvm_build_assign(FileCompiler *fc, TokenAssign *ta) {
    //
    printf("right\n");
    LLVMValueRef value = llvm_value(fc, ta->right);
    printf("left\n");
    LLVMValueRef set_on;
    //
    if (ta->left->type == vt_threaded_global) {
        // Create buffer and set at end
        die("Todo: assign GT");
    } else if (ta->left->type == vt_shared_global) {
        //
        GlobalVar *gv = ta->left->item;
        set_on = LLVMGetNamedGlobal(fc->mod, gv->name);
    } else if (ta->left->type == vt_local_var) {
        //
        char *name = ta->left->item;
        set_on = llvm_get_var(fc, name);
    } else if (ta->left->type == vt_prop_access) {
        //
        ValueClassPropAccess *pa = ta->left->item;
        Value *on = pa->on;
        LLVMValueRef onv = llvm_value(fc, on);
        set_on = llvm_build_prop_access(fc, onv, on->return_type->class, pa->name);
    } else {
        die("Codegen: left side is not assign-able");
    }

    printf("ref\n");

    // Upref new value
    Type *rtype = ta->right->return_type;
    if (rtype->class && rtype->class->ref_count) {
        llvm_upref(fc, value, rtype);
    }

    // Deref current value
    Type *ltype = ta->left->return_type;
    if (ltype->class && ltype->class->ref_count) {
        llvm_deref(fc, set_on, ltype);
    }

    printf("op\n");

    if (ta->type == op_eq) {
        //
    } else if (ta->type == op_add) {
        LLVMValueRef load = LLVMBuildLoad2(fc->builder, llvm_type(fc, ta->left->return_type), set_on, llvm_buf(fc));
        value = LLVMBuildAdd(fc->builder, load, value, llvm_buf(fc));
    } else if (ta->type == op_sub) {
        LLVMValueRef load = LLVMBuildLoad2(fc->builder, llvm_type(fc, ta->left->return_type), set_on, llvm_buf(fc));
        value = LLVMBuildSub(fc->builder, load, value, llvm_buf(fc));
    } else if (ta->type == op_mult) {
        LLVMValueRef load = LLVMBuildLoad2(fc->builder, llvm_type(fc, ta->left->return_type), set_on, llvm_buf(fc));
        value = LLVMBuildMul(fc->builder, load, value, llvm_buf(fc));
    } else if (ta->type == op_div) {
        LLVMValueRef load = LLVMBuildLoad2(fc->builder, llvm_type(fc, ta->left->return_type), set_on, llvm_buf(fc));
        if (ta->left->return_type->class->is_unsigned) {
            value = LLVMBuildUDiv(fc->builder, load, value, llvm_buf(fc));
        } else {
            value = LLVMBuildSDiv(fc->builder, load, value, llvm_buf(fc));
        }
    } else if (ta->type == op_mod) {
        LLVMValueRef load = LLVMBuildLoad2(fc->builder, llvm_type(fc, ta->left->return_type), set_on, llvm_buf(fc));
        if (ta->left->return_type->class->is_unsigned) {
            value = LLVMBuildURem(fc->builder, load, value, llvm_buf(fc));
        } else {
            value = LLVMBuildSRem(fc->builder, load, value, llvm_buf(fc));
        }
    } else if (ta->type == op_bit_OR) {
        LLVMValueRef load = LLVMBuildLoad2(fc->builder, llvm_type(fc, ta->left->return_type), set_on, llvm_buf(fc));
        value = LLVMBuildOr(fc->builder, load, value, llvm_buf(fc));
    } else if (ta->type == op_bit_AND) {
        LLVMValueRef load = LLVMBuildLoad2(fc->builder, llvm_type(fc, ta->left->return_type), set_on, llvm_buf(fc));
        value = LLVMBuildAnd(fc->builder, load, value, llvm_buf(fc));
    } else if (ta->type == op_bit_XOR) {
        LLVMValueRef load = LLVMBuildLoad2(fc->builder, llvm_type(fc, ta->left->return_type), set_on, llvm_buf(fc));
        value = LLVMBuildXor(fc->builder, load, value, llvm_buf(fc));
    } else {
        fc_error(fc, "Unhandled assign operator translation", NULL);
    }

    printf("store (assign-type:%d)\n", ta->type);
    printf("on: %s\n", LLVMPrintValueToString(set_on));
    printf("value: %s\n", LLVMPrintValueToString(value));
    LLVMBuildStore(fc->builder, value, set_on);

    //
    // if (ta->left->type == vt_threaded_global) {
    //     GlobalVar *gv = ta->left->item;
    //     str_append_chars(fc->tkn_buffer, "pthread_setspecific(");
    //     str_append_chars(fc->tkn_buffer, gv->cname);
    //     str_append_chars(fc->tkn_buffer, ", ");
    //     str_append_chars(fc->tkn_buffer, left);
    //     str_append_chars(fc->tkn_buffer, ");\n");
    // }
}