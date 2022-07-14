
#include "../all.h"

LLVMValueRef llvm_build_class_init(FileCompiler *fc, Value *value) {
    //
    fc->var_bufc++;
    ValueClassInit *ini = value->item Class *class = ini->class;
    LLVMValueRef allocator_func = llvm_get_allocator(fc, class->size, true);
    char *allocator_name = LLVMGetValueName(allocator_func);

    // Generate function
    char *func_name = malloc(60);
    sprintf(func_name, "_KI_CLASS_INIT_%s_%d", fc->hash, fc->var_bufc);

    int argc = ini->prop_values->values->length;

    LLVMTypeRef *ini_types = malloc(sizeof(LLVMTypeRef) * argc);
    LLVMValueRef *ini_values = malloc(sizeof(LLVMValueRef) * argc);
    for (int i = 0; i < ini->prop_values->values->length; i++) {
        Value *val = array_get_index(ini->prop_values->values, i);
        ini_types[i] = llvm_type(val->return_type);
        ini_values[i] = llvm_value(val->item);
    }

    LLVMTypeRef ftype = LLVMFunctionType(llvm_ptr(), ini_types, argc, 0);
    LLVMValueRef fv = LLVMAddFunction(fc->mod, func_name, ftype);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(fv, "entry");
    LLVMBasicBlockRef current_block = fc->current_block;
    LLVMBasicBlockRef current_scope = fc->current_scope;
    fc->current_scope = init_sub_scope(fc->scope);
    LLVMPositionBuilderAtEnd(fc->builder, entry);
    // Build function

    LLVMValueRef alc_call = LLVMBuildCall2(fc->builder, LLVMGetCalledFunctionType(allocator_func), allocator_func, NULL, 0, "class_init_alc_1");
    LLVMValueRef *ini_values_1 = malloc(sizeof(LLVMValueRef) * 1);
    ini_values_1[0] = alc_call;

    LLVMValueRef gc_func = LLVMGetNamedFunction(fc->mod, "ki__mem__Allocator__get_chunk");
    LLVMValueRef alcget_call = LLVMBuildCall2(fc->builder, LLVMGetCalledFunctionType(gc_func), gc_func, ini_values_1, 1, "class_init_alcget_1");

    unsigned long long offset = LLVMOffsetOfElement(g_target_data, llvm_type(value->return_type), llvm_prop_index(class, "_RC"));

    LLVMValueRef retv = llvm_build_declare(fc, llvm_class_type(class), "KI_RET_V");

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
