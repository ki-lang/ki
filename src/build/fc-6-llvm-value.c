
#include "../all.h"

LLVMValueRef llvm_build_class_init(FileCompiler *fc, Value *value) {
    //
    fc->var_bufc++;
    ValueClassInit *ini = value->item;
    Class *class = ini->class;
    LLVMValueRef allocator_func = llvm_get_allocator(fc, class->size, true);
    // const char *allocator_name = LLVMGetValueName(allocator_func);

    // Generate function
    char *func_name = malloc(60);
    sprintf(func_name, "_KI_CLASS_INIT_%s_%d", fc->hash, fc->var_bufc);

    int argc = ini->prop_values->values->length;

    LLVMTypeRef *ini_types = malloc(sizeof(LLVMTypeRef) * argc);
    LLVMValueRef *ini_values = malloc(sizeof(LLVMValueRef) * argc);
    for (int i = 0; i < ini->prop_values->values->length; i++) {
        Value *val = array_get_index(ini->prop_values->values, i);
        ini_types[i] = llvm_type(val->return_type);
        ini_values[i] = llvm_value(fc, val->item);
    }

    LLVMTypeRef ftype = LLVMFunctionType(llvm_ptr(), ini_types, argc, 0);
    LLVMValueRef fv = LLVMAddFunction(fc->mod, func_name, ftype);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(fv, "entry");
    LLVMBasicBlockRef current_block = fc->current_block;
    Scope *current_scope = fc->current_scope;
    fc->current_scope = init_sub_scope(fc->scope);
    LLVMPositionBuilderAtEnd(fc->builder, entry);
    // Build function

    LLVMValueRef alc_call = LLVMBuildCall2(fc->builder, LLVMGetCalledFunctionType(allocator_func), allocator_func, NULL, 0, "class_init_alc_1");
    LLVMValueRef *ini_values_1 = malloc(sizeof(LLVMValueRef) * 1);
    ini_values_1[0] = alc_call;

    LLVMValueRef gc_func = LLVMGetNamedFunction(fc->mod, "ki__mem__Allocator__get_chunk");
    LLVMValueRef alcget_call = LLVMBuildCall2(fc->builder, LLVMGetCalledFunctionType(gc_func), gc_func, ini_values_1, 1, "class_init_alcget_1");

    LLVMValueRef retv = llvm_build_declare(fc, llvm_class_type(class), "KI_RET_V");
    LLVMBuildStore(fc->builder, alcget_call, retv);

    if (class->ref_count) {
        LLVMValueRef v = llvm_build_prop_access(fc, retv, class, "_RC");
        LLVMBuildStore(fc->builder, llvm_int(0), v);
        // str_append_chars(fc->c_code_after, "KI_RET_V");
        // str_append_chars(fc->c_code_after, sign);
        // str_append_chars(fc->c_code_after, "_RC = 0;\n");
    }

    for (int i = 0; i < ini->prop_values->keys->length; i++) {
        char *prop_name = array_get_index(ini->prop_values->keys, i);
        Value *v = array_get_index(ini->prop_values->values, i);

        LLVMValueRef pa = llvm_build_prop_access(fc, retv, class, prop_name);
        LLVMBuildStore(fc->builder, LLVMGetParam(fv, i), pa);

        if (v->return_type->class && v->return_type->class->ref_count) {
            // KI_RET_V->{prop_name}->_RC++
            LLVMValueRef pa = llvm_build_prop_access(fc, pa, v->return_type->class, "_RC");
            llvm_upref(fc, pa, false);
        }
    }

    for (int i = 0; i < class->props->keys->length; i++) {
        char *prop_name = array_get_index(class->props->keys, i);
        if (map_contains(ini->prop_values, prop_name)) {
            continue;
        }
        ClassProp *prop = array_get_index(class->props->values, i);
        if (!prop->default_value) {
            continue;
        }

        LLVMValueRef v = llvm_value(fc, prop->default_value);
        LLVMValueRef pa = llvm_build_prop_access(fc, retv, class, prop_name);
        LLVMBuildStore(fc->builder, v, pa);
    }

    llvm_deref_local_vars(fc, NULL, fc->current_scope);

    // Build call
    fc->current_block = current_block;
    fc->current_scope = current_scope;
    //
    LLVMPositionBuilderAtEnd(fc->builder, current_block);
    LLVMValueRef init_call = LLVMBuildCall2(fc->builder, ftype, fv, ini_values, argc, "class_init_1");

    return init_call;
}

LLVMValueRef llvm_build_func_call(FileCompiler *fc, Value *value) {
    //
    ValueFuncCall *fa = value->item;

    int argc = fa->arg_values->length;
    if (fa->ort != NULL) {
        argc++;
    }
    LLVMValueRef *args = malloc(sizeof(LLVMValueRef) * argc);
    for (int i = 0; i < fa->arg_values->length; i++) {
        Value *v = array_get_index(fa->arg_values, i);
        args[i] = llvm_value(fc, v);
    }
    if (fa->ort != NULL) {
        LLVMValueRef var = llvm_get_var(fc, "_KI_THROW_MSG_BUF");
        LLVMValueRef ptr = llvm_build_declare(fc, llvm_ptr(), "tmp");
        ptr = LLVMBuildGEP2(fc->builder, llvm_ptr(), var, NULL, 0, "tmp");
        args[argc - 1] = ptr;
    }

    LLVMTypeRef ont = llvm_type(fa->on->return_type);
    LLVMValueRef onv = llvm_value(fc, fa->on);
    LLVMValueRef retv = LLVMBuildCall2(fc->builder, ont, onv, args, argc, "fcall_1");

    if (fa->ort != NULL) {

        LLVMTypeRef rett = llvm_type(value->return_type);
        char *ret_var = strdup(var_buf(fc));
        LLVMValueRef ort_retv = llvm_build_declare(fc, rett, ret_var);
        LLVMBuildStore(fc->builder, retv, ort_retv);

        //
        LLVMValueRef throw_var = llvm_get_var(fc, "_KI_THROW_MSG_BUF");
        LLVMValueRef cond = llvm_icmp(fc, throw_var, llvm_int(0));
        LLVMBasicBlockRef then = LLVMAppendBasicBlock(fc->current_func, "fcall_ort");
        LLVMBasicBlockRef next = LLVMAppendBasicBlock(fc->current_func, "fcall_ort_next");
        LLVMBuildCondBr(fc->builder, cond, then, NULL);
        //
        LLVMPositionBuilderAtEnd(fc->builder, then);
        fc->current_block = then;

        if (fa->ort->type != or_pass) {
            LLVMValueRef zero = llvm_int(0);
            LLVMBuildStore(fc->builder, zero, throw_var);
        }

        LLVMValueRef ortv = llvm_build_ort(fc, fa->ort);
        if (ortv) {
            LLVMBuildStore(fc->builder, ortv, ort_retv);
        }

        LLVMPositionBuilderAtEnd(fc->builder, next);
        fc->current_block = next;

        retv = ort_retv;
    }

    // Ref count
    if (value->return_type) {
        Class *retClass = value->return_type->class;
        if (retClass && retClass->ref_count) {
            // todo
        }
    }

    return retv;
    /*

            if (value->return_type) {
                Class *retClass = value->return_type->class;
                if (retClass && retClass->ref_count) {
                    // Buffer the value
                    char *buf_var_name = strdup(var_buf(fc));
                    str_append_chars(code, "struct ");
                    str_append_chars(code, retClass->cname);
                    str_append_chars(code, "* ");
                    str_append_chars(code, buf_var_name);
                    str_append_chars(code, " = ");
                    str_append(code, result);
                    str_append_chars(code, ";\n");

                    if (value->return_type->nullable) {
                        str_append_chars(code, "if(");
                        str_append_chars(code, buf_var_name);
                        str_append_chars(code, ") ");
                    }

                    str_append_chars(code, buf_var_name);
                    str_append_chars(code, "->_RC++;\n");
                    result->length = 0;
                    str_append_chars(result, buf_var_name);

                    VarInfo *vi = malloc(sizeof(VarInfo));
                    vi->name = buf_var_name;
                    vi->return_type = value->return_type;

                    if (fc->current_scope)
                        array_push(fc->current_scope->var_bufs, vi);
                }
            }
                            */
}

LLVMValueRef llvm_build_ort(FileCompiler *fc, OrToken *ort) {
    //
    return NULL;
}

LLVMValueRef llvm_build_operator(FileCompiler *fc, Value *value) {

    ValueOperator *op = value->item;
    LLVMValueRef left = llvm_value(fc, op->left);
    LLVMValueRef right = NULL;
    if (op->right && op->type != op_and && op->type != op_or) {
        right = llvm_value(fc, op->right);
    }

    if (op->type == op_add) {
        return LLVMBuildAdd(fc->builder, left, right, llvm_buf(fc));
    } else if (op->type == op_sub) {
        return LLVMBuildSub(fc->builder, left, right, llvm_buf(fc));
    } else if (op->type == op_mult) {
        return LLVMBuildMul(fc->builder, left, right, llvm_buf(fc));
    } else if (op->type == op_div) {
        return LLVMBuildFDiv(fc->builder, left, right, llvm_buf(fc));
    } else if (op->type == op_mod) {
        return LLVMBuildSRem(fc->builder, left, right, llvm_buf(fc));
    } else if (op->type == op_bit_OR) {
        return LLVMBuildOr(fc->builder, left, right, llvm_buf(fc));
    } else if (op->type == op_bit_AND) {
        return LLVMBuildAnd(fc->builder, left, right, llvm_buf(fc));
    } else if (op->type == op_bit_XOR) {
        return LLVMBuildXor(fc->builder, left, right, llvm_buf(fc));
    } else if (op->type == op_bit_shift_left) {
        return LLVMBuildShl(fc->builder, left, right, llvm_buf(fc));
    } else if (op->type == op_bit_shift_right) {
        return LLVMBuildAShr(fc->builder, left, right, llvm_buf(fc));
        //
    } else if (op->type == op_and || op->type == op_or) {
        // Create blocks
        LLVMBasicBlockRef second_check = LLVMAppendBasicBlock(fc->current_func, llvm_buf(fc));
        LLVMBasicBlockRef after = LLVMAppendBasicBlock(fc->current_func, llvm_buf(fc));
        // First
        LLVMValueRef ifv = LLVMBuildICmp(fc->builder, LLVMIntNE, left, llvm_u8(0), llvm_buf(fc));
        if (op->type == op_and) {
            // &&
            LLVMBuildCondBr(fc->builder, ifv, second_check, after);
        } else {
            // ||
            LLVMBuildCondBr(fc->builder, ifv, after, second_check);
        }
        // Second
        LLVMPositionBuilderAtEnd(fc->builder, second_check);
        right = llvm_value(fc, op->right);
        LLVMBuildICmp(fc->builder, LLVMIntNE, right, llvm_u8(0), llvm_buf(fc));
        LLVMBuildBr(fc->builder, after);
        // Result
        LLVMValueRef phi = LLVMBuildPhi(fc->builder, LLVMInt1Type(), llvm_buf(fc));
        return LLVMBuildZExt(fc->builder, phi, LLVMInt8Type(), llvm_buf(fc));
        //
    } else if (op->type == op_eq) {
        LLVMValueRef cmp = LLVMBuildICmp(fc->builder, LLVMIntEQ, left, right, llvm_buf(fc));
        return LLVMBuildZExt(fc->builder, cmp, LLVMInt8Type(), llvm_buf(fc));
    } else if (op->type == op_neq) {
        LLVMValueRef cmp = LLVMBuildICmp(fc->builder, LLVMIntNE, left, right, llvm_buf(fc));
        return LLVMBuildZExt(fc->builder, cmp, LLVMInt8Type(), llvm_buf(fc));
    } else if (op->type == op_lt) {
        LLVMValueRef cmp = LLVMBuildICmp(fc->builder, LLVMIntSLT, left, right, llvm_buf(fc));
        return LLVMBuildZExt(fc->builder, cmp, LLVMInt8Type(), llvm_buf(fc));
    } else if (op->type == op_lte) {
        LLVMValueRef cmp = LLVMBuildICmp(fc->builder, LLVMIntSLE, left, right, llvm_buf(fc));
        return LLVMBuildZExt(fc->builder, cmp, LLVMInt8Type(), llvm_buf(fc));
    } else if (op->type == op_gt) {
        LLVMValueRef cmp = LLVMBuildICmp(fc->builder, LLVMIntSGT, left, right, llvm_buf(fc));
        return LLVMBuildZExt(fc->builder, cmp, LLVMInt8Type(), llvm_buf(fc));
    } else if (op->type == op_gte) {
        LLVMValueRef cmp = LLVMBuildICmp(fc->builder, LLVMIntSGE, left, right, llvm_buf(fc));
        return LLVMBuildZExt(fc->builder, cmp, LLVMInt8Type(), llvm_buf(fc));
    } else if (op->type == op_incr) {
        return LLVMBuildAdd(fc->builder, left, llvm_int(1), llvm_buf(fc));
    } else if (op->type == op_decr) {
        return LLVMBuildSub(fc->builder, left, llvm_int(1), llvm_buf(fc));
    } else {
        printf("Op: %d\n", op->type);
        fc_error(fc, "Unhandled operator type", NULL);
    }
    return NULL;
}
