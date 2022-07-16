
#include "../all.h"

//
LLVMModuleRef g_llvm_inits_mod;
LLVMBuilderRef g_llvm_inits_builder;
LLVMValueRef g_llvm_inits;
LLVMTypeRef g_llvm_inits_type;
LLVMValueRef g_llvm_init_thread;
LLVMTypeRef g_llvm_init_thread_type;
//

void fc_write_c_all() {
    // Clear header
    char *path = malloc(KI_PATH_MAX);
    char *cache_dir = get_cache_dir();

    // #if defined __APPLE__ || defined _WIN32
    //     write_file(path, "extern int errno;\n", true);
    // #else
    //     write_file(path, "extern __thread int errno;\n", true);
    // #endif

    llvm_set_target();

    llvm_build_inits();

    strcpy(path, cache_dir);
    strcat(path, "/inits.o");
    llvm_build_o(g_llvm_inits_mod, path);

    for (int i = 0; i < packages->keys->length; i++) {
        PkgCompiler *pkc = array_get_index(packages->values, i);
        for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
            FileCompiler *fc = array_get_index(pkc->file_compilers->values, o);
            if (fc->should_recompile && !fc->is_header) {

                printf("Compile: %s\n", fc->ki_filepath);

                fc->mod = LLVMModuleCreateWithName("main_module");
                fc->builder = LLVMCreateBuilder();

                // Pre define all functions
                for (int x = 0; x < fc->functions->length; x++) {
                    Function *func = array_get_index(fc->functions, x);
                    llvm_define_func(fc, func);
                }

                // Build functions
                for (int x = 0; x < fc->functions->length; x++) {
                    Function *func = array_get_index(fc->functions, x);
                    llvm_build_func(fc, func);
                }

                llvm_build_o(fc->mod, fc->o_filepath);
            }
        }
    }

    // Predefine all struct names
    // for (int i = 0; i < packages->keys->length; i++) {
    //     PkgCompiler *pkc = array_get_index(packages->values, i);
    //     for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
    //         FileCompiler *fc = array_get_index(pkc->file_compilers->values, o);

    //         for (int x = 0; x < fc->classes->length; x++) {
    //             Class *class = array_get_index(fc->classes, x);
    //             if (class->generic_names != NULL && class->generic_hash == NULL) {
    //                 continue;
    //             }
    //             fc_write_c_predefine_class(fc, class);
    //         }

    //         fc_write_c_pre(fc);
    //     }
    // }

    //
    for (int i = 0; i < packages->keys->length; i++) {
        PkgCompiler *pkc = array_get_index(packages->values, i);
        for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
            FileCompiler *fc = array_get_index(pkc->file_compilers->values, o);

            if (fc->should_recompile) {

                // for (int x = 0; x < fc->classes->length; x++) {
                //     Class *class = array_get_index(fc->classes, x);
                //     if (class->generic_names != NULL && class->generic_hash == NULL) {
                //         continue;
                //     }
                //     fc_write_c_class(fc, class);
                // }
                // for (int x = 0; x < fc->enums->length; x++) {
                //     fc_write_c_enum(fc, array_get_index(fc->enums, x));
                // }
                // for (int x = 0; x < fc->globals->length; x++) {
                //     fc_write_c_global(fc, array_get_index(fc->globals, x));
                // }
                // for (int x = 0; x < fc->strings->length; x++) {
                //     ValueString *vstr = array_get_index(fc->strings, x);
                //     str_append_chars(fc->h_code, "struct ki__type__String* ");
                //     str_append_chars(fc->h_code, vstr->name);
                //     str_append_chars(fc->h_code, ";\n");
                // }

                // fc_write_c_ast(fc, fc->scope);
            }

            // fc_write_c(fc);
        }
    }

    // fc_write_c_inits();

    // if (uses_async) {
    //     write_file(path, "void ki__async__Taskman__add_task(struct ki__async__Task* task);\n", true);
    //     write_file(path, "void ki__async__Taskman__run_another_task();\n", true);
    // }
    // write_file(path, "void KI_INITS();\n", true);
    // write_file(path, "void KI_INIT_THREAD();\n", true);
    // write_file(path, "void* KI_ALLOCATORS;\n", true);
    // write_file(path, "void* KI_ALLOCATORS_MUT;\n", true);

    LLVMDisposeTargetMachine(g_target_machine);
}

void llvm_set_target() {
    //
    char *error = NULL;
    //
    LLVMInitializeNativeTarget();
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmParsers();
    LLVMInitializeAllAsmPrinters();
    //
    LLVMTargetRef target;
    if (LLVMGetTargetFromTriple(LLVM_DEFAULT_TARGET_TRIPLE, &target, &error) != 0) {
        fprintf(stderr, "Failed to get target!\n");
        fprintf(stderr, "%s\n", error);
        LLVMDisposeMessage(error);
        exit(1);
    }

    char *cpu = "generic";
    char *features = "";

    g_target_machine = LLVMCreateTargetMachine(target, LLVM_DEFAULT_TARGET_TRIPLE, cpu, features, LLVMCodeGenLevelDefault, LLVMRelocDefault, LLVMCodeModelDefault);
    g_target_data = LLVMCreateTargetDataLayout(g_target_machine);
}

void llvm_build_inits() {
    //
    LLVMModuleRef mod = LLVMModuleCreateWithName("main_module");
    LLVMBuilderRef b = LLVMCreateBuilder();
    g_llvm_inits_mod = mod;
    g_llvm_inits_builder = b;

    // Inits
    g_llvm_inits_type = LLVMFunctionType(LLVMVoidType(), NULL, 0, 0);
    g_llvm_inits = LLVMAddFunction(g_llvm_inits_mod, "KI_INITS", g_llvm_inits_type);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(g_llvm_init_thread, "entry");
    LLVMPositionBuilderAtEnd(b, entry);
    LLVMBuildRetVoid(b);

    // Init thread
    g_llvm_init_thread_type = LLVMFunctionType(LLVMVoidType(), NULL, 0, 0);
    g_llvm_inits = LLVMAddFunction(g_llvm_inits_mod, "KI_INIT_THREAD", g_llvm_init_thread_type);
    entry = LLVMAppendBasicBlock(g_llvm_init_thread, "entry");
    LLVMPositionBuilderAtEnd(b, entry);
    LLVMBuildRetVoid(b);
}

void llvm_build_ast(FileCompiler *fc, Scope *scope) {
    //
    Scope *prev_scope = fc->current_scope;
    fc->current_scope = scope;

    int c = 0;
    Array *ast = scope->ast;
    while (c < ast->length) {
        Token *t = array_get_index(ast, c);
        llvm_build_token(fc, t);
        c++;
    }

    if (!scope->did_return) {
        llvm_deref_local_vars(fc, NULL, fc->current_scope);
    }

    fc->current_scope = prev_scope;
}

void llvm_define_func(FileCompiler *fc, Function *func) {
    printf("pre: %s\n", func->cname);
    LLVMTypeRef param_types[func->arg_types->length];
    for (int i = 0; i < func->arg_types->length; i++) {
        param_types[i] = llvm_type(array_get_index(func->arg_types, i));
    }
    LLVMTypeRef ftype = LLVMFunctionType(LLVMInt32Type(), param_types, func->arg_types->length, 0);
    LLVMValueRef llvmfn = LLVMAddFunction(fc->mod, func->cname, ftype);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(llvmfn, "entry");
}

void llvm_build_func(FileCompiler *fc, Function *func) {
    //
    printf("func: %s\n", func->cname);
    fc->llvmc = 0;
    //
    LLVMValueRef llvmfn = LLVMGetNamedFunction(fc->mod, func->cname);
    LLVMBasicBlockRef entry = LLVMGetEntryBasicBlock(llvmfn);

    LLVMPositionBuilderAtEnd(fc->builder, entry);

    llvm_build_ast(fc, func->scope);

    // hasReturn = LLVMGetBasicBlockTerminator(LLVMGetLastBasicBlock(llvmfn)) != NULL;
    if (!func->return_type) {
        LLVMBuildRetVoid(fc->builder);
    }
}

LLVMValueRef llvm_build_declare(FileCompiler *fc, LLVMTypeRef type, char *name) {
    //
    LLVMValueRef a = LLVMBuildAlloca(fc->builder, type, name);
    Scope *scope = fc->current_scope;
    map_set(scope->llvm_declares, name, a);
    return a;
}

LLVMValueRef llvm_get_var(FileCompiler *fc, char *name) {
    //
    Scope *scope = fc->current_scope;
    while (scope) {
        LLVMValueRef v = map_get(scope->llvm_declares, name);
        if (v) {
            return v;
        }
        scope = scope->parent;
    }
    printf("Var name: %s\n", name);
    die("Variable not found");
    return NULL;
}

LLVMValueRef llvm_get_func(FileCompiler *fc, char *name, Type *func_ref_type) {
    //
    LLVMValueRef func = LLVMGetNamedFunction(fc->mod, name);
    if (!func) {
        // Func is extern
        printf("extern-func: %s\n", name);
        char *x = type_to_str(func_ref_type);
        printf("extern-func-type: %s\n", x);
        func = LLVMAddFunction(fc->mod, name, llvm_type(func_ref_type));
    }
    return func;
}

LLVMValueRef llvm_build_prop_access(FileCompiler *fc, LLVMValueRef on, Class *class, char *prop_name) {
    //
    ClassProp *prop = map_get(class->struct_props, prop_name);
    return LLVMBuildStructGEP2(fc->builder, class->llvm_type, on, prop->struct_index, llvm_buf(fc));
}

LLVMTypeRef llvm_type(Type *type) {
    //
    LLVMTypeRef result;
    if (type->is_pointer) {
        result = LLVMIntPtrType(g_target_data);
    } else if (type->type == type_void) {
        result = LLVMVoidType();
    } else if (type->type == type_void_pointer) {
        result = LLVMVoidType();
    } else if (type->type == type_funcref) {
        return llvm_funcref_type(type);
    } else if (type->type == type_struct) {
        Class *class = type->class;

        if (class->is_number) {
            int bytes = type->bytes;

            if (bytes == 1) {
                result = LLVMInt8Type();
            } else if (bytes == 2) {
                result = LLVMInt16Type();
            } else if (bytes == 4) {
                result = LLVMInt32Type();
            } else if (bytes == 8) {
                result = LLVMInt64Type();
            } else {
                die("Cant determine llvm number type based on the amount of bytes");
            }

        } else {
            result = llvm_class_type(class);
        }

    } else if (type->type == type_enum) {
        result = LLVMInt32Type();
    } else if (type->type == type_bool) {
        result = LLVMInt8Type();
    } else {
        printf("Type: %d\n", type->type);
        die("Cannot convert llvm type");
    }

    printf("FROM: %s\n", type_to_str(type));
    printf("TO: %s\n", LLVMPrintTypeToString(result));

    return result;
}

LLVMTypeRef llvm_funcref_type(Type *type) {
    //
    int argc = type->func_arg_types->length;
    LLVMTypeRef param_types[argc];
    for (int i = 0; i < argc; i++) {
        param_types[i] = llvm_type(array_get_index(type->func_arg_types, i));
    }
    return LLVMFunctionType(llvm_type(type->func_return_type), param_types, argc, 0);
}

LLVMTypeRef llvm_class_type(Class *class) {
    //
    if (class->llvm_type == NULL) {

        int propc = class->struct_props->values->length;
        LLVMTypeRef *prop_types = malloc(sizeof(LLVMTypeRef) * propc);
        class->llvm_type = LLVMStructType(prop_types, propc, 0);
        //
        for (int i = 0; i < propc; i++) {
            ClassProp *prop = array_get_index(class->struct_props->values, i);
            prop_types[i] = llvm_type(prop->return_type);
        }
    }

    return class->llvm_type;
}

LLVMValueRef llvm_value(FileCompiler *fc, Value *value) {
    //
    LLVMValueRef result = NULL;

    if (value->type == vt_null) {
        return llvm_null();
    } else if (value->type == vt_false) {
        return LLVMConstInt(LLVMInt8Type(), 0, false);
    } else if (value->type == vt_true) {
        return LLVMConstInt(LLVMInt8Type(), 1, false);
    } else if (value->type == vt_group) {
        // str_append_chars(result, "(");
        return llvm_value(fc, value->item);
        // str_append_chars(result, ")");
    } else if (value->type == vt_shared_global) {
        return LLVMGetNamedGlobal(fc->mod, value->item);
    } else if (value->type == vt_class_init) {
        return llvm_build_class_init(fc, value->item);
    } else if (value->type == vt_number) {
        printf("num\n");
        ValueNumber *vn = value->item;
        if (vn->is_float) {
            die("TODO: float");
        } else {
            return LLVMConstInt(LLVMInt32Type(), vn->iv, true);
        }
    } else if (value->type == vt_func_call) {
        return llvm_build_func_call(fc, value);
    } else if (value->type == vt_operator) {
        printf("op\n");
        return llvm_build_operator(fc, value);
    } else if (value->type == vt_int) {
        printf("int\n");
        return llvm_int((intptr_t)value->item);
    } else if (value->type == vt_prop_access) {

        ValueClassPropAccess *pa = value->item;

        if (pa->is_static) {
            Class *class = pa->on;
            ClassProp *prop = map_get(class->props, pa->name);

            char name[KI_TOKEN_MAX];
            strcpy(name, class->cname);
            strcat(name, "__");
            strcat(name, pa->name);

            if (prop->is_func) {
                printf("static-func: %s\n", name);
                return llvm_get_func(fc, name, prop->return_type);
            } else {
                die("Static variables do not exist");
            }

        } else {
            Value *val = pa->on;

            LLVMValueRef on = llvm_value(fc, val);

            Type *type = val->return_type;
            if (type->is_pointer) {
                // %3 = load %struct.A*, %struct.A** %1, align 8
                on = LLVMBuildLoad2(fc->builder, llvm_type(type), on, llvm_buf(fc));
            }
            // %4 = getelementptr inbounds %struct.A, %struct.A* %3, i32 0, i32 1
            // %5 = load i32, i32* %4, align 4
            on = llvm_build_prop_access(fc, on, type->class, pa->name);
            return LLVMBuildLoad2(fc->builder, llvm_type(value->return_type), on, llvm_buf(fc));
        }
        /*

    } else if (value->type == vt_nullable_value) {
        //
        Type *ret = value->return_type;
        str_append_chars(result, ret->class->cname);
        str_append_chars(result, "__init(");
        fc_write_c_value(fc, value->item, false, code);
        str_append_chars(result, ")");
            } else if (value->type == vt_threaded_global) {
                //
                GlobalVar *gv = value->item;

                str_append_chars(result, "pthread_getspecific(");
                str_append_chars(result, gv->cname);
                str_append_chars(result, ")");
                //
            } else if (value->type == vt_shared_global) {
                //
                GlobalVar *gv = value->item;
                str_append_chars(result, gv->cname);
                //
            } else if (value->type == vt_arg) {
                str_append_chars(result, value->item);
            } else if (value->type == vt_number) {
                str_append_chars(result, value->item);
            } else if (value->type == vt_char) {
                str_append_chars(result, "'");
                str_append_chars(result, value->item);
                str_append_chars(result, "'");
            } else if (value->type == vt_null_or) {
                ValueOperator *op = value->item;
                fc_write_c_value(fc, op->left, true, code);
                char *buf_var_name = strdup(var_buf(fc));
                fc_write_c_type(code, op->left->return_type, buf_var_name);
                str_append_chars(code, " = ");
                str_append(code, fc->value_buffer);
                str_append_chars(code, ";\n");
                //
                str_append_chars(code, "if(");
                str_append_chars(code, buf_var_name);
                str_append_chars(code, " == (void*)0) {\n");

                str_append_chars(code, buf_var_name);
                str_append_chars(code, " = ");
                fc_write_c_value(fc, op->right, true, code);
                str_append(code, fc->value_buffer);
                str_append_chars(code, ";\n");

                str_append_chars(code, "}\n");

                result->length = 0;
                str_append_chars(result, buf_var_name);
                free(buf_var_name);

            } else if (value->type == vt_func_call) {

            } else if (value->type == vt_cast) {
                ValueCast *cast = value->item;
                str_append_chars(result, "(");
                fc_write_c_type(result, cast->as_type, NULL);
                str_append_chars(result, ")");
                fc_write_c_value(fc, cast->value, false, code);
            } else if (value->type == vt_getptrv) {
                ValueCast *cast = value->item;
                str_append_chars(result, "(*(");
                fc_write_c_type(result, cast->as_type, NULL);
                str_append_chars(result, "*)(");
                fc_write_c_value(fc, cast->value, false, code);
                str_append_chars(result, "))");
            } else if (value->type == vt_getptr) {
                str_append_chars(result, "&");
                fc_write_c_value(fc, value->item, false, code);
            } else if (value->type == vt_setptrv) {
                SetPtrValue *cast = value->item;
                str_append_chars(result, "*(");
                fc_write_c_type(result, cast->to_value->return_type, NULL);
                str_append_chars(result, "*)");
                fc_write_c_value(fc, cast->ptr_value, false, code);
                str_append_chars(result, " = ");
                fc_write_c_value(fc, cast->to_value, false, code);
            } else if (value->type == vt_async) {
                Value *fcallv = value->item;
                ValueFuncCall *fcall = fcallv->item;
                Value *on = fcall->on;
                char *size = malloc(10);
                //
                Type *task_type = fc_identifier_to_type(fc, create_identifier("ki", "async", "Task"), NULL);
                // Cache current value
                char *cache = str_to_chars(fc->value_buffer);

                // Step 1. Generate execution function
                char *handler_name = strdup(var_buf(fc));
                str_append_chars(fc->c_code_after, "void ");
                str_append_chars(fc->c_code_after, handler_name);
                str_append_chars(fc->c_code_after, "(");
                fc_write_c_type(fc->c_code_after, task_type, "task");
                str_append_chars(fc->c_code_after, ") {\n");
                str_append_chars(fc->c_code_after, "void* arg_pointer = task->args;\n");
                // Header
                str_append_chars(fc->h_code, "void ");
                str_append_chars(fc->h_code, handler_name);
                str_append_chars(fc->h_code, "(");
                fc_write_c_type(fc->h_code, task_type, "task");
                str_append_chars(fc->h_code, ");\n");

                // Body
                int args_size = 0;
                Array *arg_strings = array_make(4);
                for (int i = 0; i < fcall->arg_values->length; i++) {
                    char *arg_name = malloc(20);
                    Value *v = array_get_index(fcall->arg_values, i);
                    args_size += v->return_type->bytes;
                    sprintf(arg_name, "arg_%d", i);
                    array_push(arg_strings, arg_name);
                    fc_write_c_type(fc->c_code_after, v->return_type, arg_name);
                    str_append_chars(fc->c_code_after, " = *(");
                    v->return_type->is_pointer_of_pointer = true;
                    fc_write_c_type(fc->c_code_after, v->return_type, NULL);
                    v->return_type->is_pointer_of_pointer = false;
                    str_append_chars(fc->c_code_after, ")arg_pointer;\n");
                    str_append_chars(fc->c_code_after, "arg_pointer += ");
                    sprintf(size, "%d", v->return_type->bytes);
                    str_append_chars(fc->c_code_after, size);
                    str_append_chars(fc->c_code_after, ";\n");
                }
                // Call func
                char *func_ref_name = strdup(var_buf(fc));
                str_append_chars(fc->c_code_after, "void* ");
                str_append_chars(fc->c_code_after, func_ref_name);
                str_append_chars(fc->c_code_after, " = task->func;\n");

                char *ret_name = NULL;
                if (fcallv->return_type) {
                    ret_name = var_buf(fc);
                    fc_write_c_type(fc->c_code_after, fcallv->return_type, ret_name);
                    str_append_chars(fc->c_code_after, " = ");
                }
                str_append_chars(fc->c_code_after, "((");
                fc_write_c_type(fc->c_code_after, on->return_type, NULL);
                str_append_chars(fc->c_code_after, ")");
                str_append_chars(fc->c_code_after, func_ref_name);
                free(func_ref_name);
                str_append_chars(fc->c_code_after, ")(");
                // Args
                for (int i = 0; i < arg_strings->length; i++) {
                    char *arg_name = array_get_index(arg_strings, i);
                    if (i > 0) {
                        str_append_chars(fc->c_code_after, ", ");
                    }
                    str_append_chars(fc->c_code_after, arg_name);
                }
                str_append_chars(fc->c_code_after, ");\n");
                // End func call
                if (ret_name) {
                    str_append_chars(fc->c_code_after, "task->result = (void*)");
                    str_append_chars(fc->c_code_after, ret_name);
                    str_append_chars(fc->c_code_after, ";\n");
                }
                str_append_chars(fc->c_code_after, "task->ready = 1;\n");
                // Deref args if needed
                for (int i = 0; i < fcall->arg_values->length; i++) {
                    Value *v = array_get_index(fcall->arg_values, i);
                    char *arg_name = array_get_index(arg_strings, i);
                    if (v->return_type->class && v->return_type->class->ref_count) {
                        str_append_chars(fc->c_code_after, "if(--");
                        str_append_chars(fc->c_code_after, arg_name);
                        str_append_chars(fc->c_code_after, "->_RC == 0) ");
                        str_append_chars(fc->c_code_after, v->return_type->class->cname);
                        str_append_chars(fc->c_code_after, "____free(");
                        str_append_chars(fc->c_code_after, arg_name);
                        str_append_chars(fc->c_code_after, ");\n");
                    }
                }
                array_free(arg_strings);
                // End body
                str_append_chars(fc->c_code_after, "}\n\n");

                // Step 2. Create Task and push onto stack
                // Func ref
                char *allocator_name = fc_write_c_get_allocator(fc, task_type->class->size, false);
                char *func_name = strdup(var_buf(fc));
                str_append_chars(code, "void* ");
                str_append_chars(code, func_name);
                str_append_chars(code, " = ");
                fc->value_buffer->length = 0;
                fc_write_c_value(fc, on, false, code);
                str_append(code, fc->value_buffer);
                str_append_chars(code, ";\n");
                // Init Task
                char *var_name = strdup(var_buf(fc));
                fc_write_c_type(code, task_type, var_name);
                str_append_chars(code, " = ki__mem__Allocator__get_chunk(");
                str_append_chars(code, allocator_name);
                str_append_chars(code, "());\n");
                //
                str_append_chars(code, var_name);
                str_append_chars(code, "->handler_func = ");
                str_append_chars(code, handler_name);
                str_append_chars(code, ";\n");
                // Set function
                str_append_chars(code, var_name);
                str_append_chars(code, "->func = ");
                str_append_chars(code, func_name);
                str_append_chars(code, ";\n");
                // Set args
                str_append_chars(code, var_name);
                str_append_chars(code, "->args = ki__mem__alloc_flat(");
                sprintf(size, "%d", args_size);
                str_append_chars(code, size);
                str_append_chars(code, ");\n");

                char *argsptr_name = var_buf(fc);
                str_append_chars(code, "void* ");
                str_append_chars(code, argsptr_name);
                str_append_chars(code, " = ");
                str_append_chars(code, var_name);
                str_append_chars(code, "->args;\n");

                for (int i = 0; i < fcall->arg_values->length; i++) {
                    Value *v = array_get_index(fcall->arg_values, i);
                    fc->value_buffer->length = 0;
                    fc_write_c_value(fc, v, false, code);
                    //
                    str_append_chars(code, "*(");
                    v->return_type->is_pointer_of_pointer = true;
                    fc_write_c_type(code, v->return_type, NULL);
                    v->return_type->is_pointer_of_pointer = false;
                    str_append_chars(code, ")");
                    str_append_chars(code, argsptr_name);
                    str_append_chars(code, " = ");
                    str_append(code, fc->value_buffer);
                    str_append_chars(code, ";\n");

                    if (v->return_type->class && v->return_type->class->ref_count) {
                        str_append(code, fc->value_buffer);
                        str_append_chars(code, "->_RC++;\n");
                    }

                    str_append_chars(code, argsptr_name);
                    str_append_chars(code, " += ");
                    sprintf(size, "%d", v->return_type->bytes);
                    str_append_chars(code, size);
                    str_append_chars(code, ";\n");
                }

                // Push task on stack
                str_append_chars(code, "ki__async__Taskman__add_task(");
                str_append_chars(code, var_name);
                str_append_chars(code, ");\n");

                // Set cache back
                fc->value_buffer->length = 0;
                str_append_chars(fc->value_buffer, cache);
                free(cache);
                free(size);
                free(func_name);
                free(handler_name);

                str_append_chars(result, var_name);

                // free(var_name);
            } else if (value->type == vt_await) {
                // loop until task is ready
                // if not ready, check for other tasks todo
                fc_write_c_value(fc, value->item, false, code);
                //
                str_append_chars(code, "while(!");
                str_append(code, result);
                str_append_chars(code, "->ready){\n");
                str_append_chars(code, "ki__async__Taskman__run_another_task();\n");
                str_append_chars(code, "}\n");

                str_append_chars(result, "->result");
                //
            } else if (value->type == vt_allocator) {
                char *size = value->item;
                int sizei = atoi(size);
                char *name = fc_write_c_get_allocator(fc, sizei, true);
                str_append_chars(result, name);
                str_append_chars(result, "()");
                */

    } else {
        printf("Value token type: %d\n", value->type);
        die("Unhandled value token");
    }

    //
    return result;
}

void llvm_build_token(FileCompiler *fc, Token *token) {
    //
    if (token->type == tkn_func) {
        llvm_build_func(fc, token->item);
    } else if (token->type == tkn_init_thread) {
        LLVMBuildCall2(fc->builder, g_llvm_init_thread_type, g_llvm_init_thread, NULL, 0, NULL);
    } else if (token->type == tkn_return) {

        Value *retv = NULL;
        if (token->item) {
            retv = token->item;
        }

        // Deref local vars + Check if var_bufs RC == 0 (if so free)
        llvm_deref_local_vars(fc, retv, NULL);

        LLVMValueRef llvm_v = llvm_value(fc, retv);
        LLVMBuildRet(fc->builder, llvm_v);
        //
    } else if (token->type == tkn_declare) {
        TokenDeclare *decl = token->item;

        LLVMValueRef v = llvm_value(fc, decl->value);
        LLVMValueRef a = llvm_build_declare(fc, llvm_type(decl->type), decl->name);
        LLVMBuildStore(fc->builder, a, v);

        // fc_write_c_type(fc->tkn_buffer, decl->type, decl->name);
        // str_append_chars(fc->tkn_buffer, " = ");
        // str_append(fc->tkn_buffer, fc->value_buffer);
        // str_append_chars(fc->tkn_buffer, ";\n");

        Class *class = decl->value->return_type->class;
        if (class && class->ref_count) {
            llvm_upref(fc, a, decl->type);
            array_push(fc->current_scope->local_var_names, decl);
        }
    } else if (token->type == tkn_if) {
        /*
    } else if (token->type == tkn_debug_msg) {
        char *msg = token->item;
        str_append_chars(fc->tkn_buffer, "write(1, \"");
        str_append_chars(fc->tkn_buffer, msg);
        str_append_chars(fc->tkn_buffer, "\\n\", ");
        sprintf(fc->sprintf, "%ld", strlen(msg) + 1);
        str_append_chars(fc->tkn_buffer, fc->sprintf);
        str_append_chars(fc->tkn_buffer, ");\n");
        // TODO: Delete errno stuff
        str_append_chars(fc->tkn_buffer, "write(1, \"errno:\", 6);\n");
        str_append_chars(fc->tkn_buffer, "struct ki__type__String* ERRNOMSG = ki__type__i32__str(errno);\n");
        str_append_chars(fc->tkn_buffer, "write(1, ERRNOMSG->data, ERRNOMSG->bytes);\n");
        str_append_chars(fc->tkn_buffer, "write(1, \"\\n\", 1);\n");
    } else if (token->type == tkn_exit) {
        ErrorToken *err = token->item;
        str_append_chars(fc->tkn_buffer, "ki__sys__exit(");
        str_append_chars(fc->tkn_buffer, err->msg);
        str_append_chars(fc->tkn_buffer, ");\n");
    } else if (token->type == tkn_panic) {
        ErrorToken *err = token->item;
        str_append_chars(fc->tkn_buffer, "write(1, \"");
        str_append_chars(fc->tkn_buffer, err->msg);
        str_append_chars(fc->tkn_buffer, "\", ");
        sprintf(fc->sprintf, "%ld", strlen(err->msg));
        str_append_chars(fc->tkn_buffer, fc->sprintf);
        str_append_chars(fc->tkn_buffer, ");\n");
        str_append_chars(fc->tkn_buffer, "ki__sys__exit(1);\n");
    } else if (token->type == tkn_each) {
        TokenEach *te = token->item;
        fc_write_c_value(fc, te->value, true, fc->tkn_buffer);
        Class *class = te->value->return_type->class;
        ClassProp *fcountp = map_get(class->props, "__each_count");
        ClassProp *fgetp = map_get(class->props, "__each_get");
        Function *fcount = fcountp->func;
        Function *fget = fgetp->func;

        char *buf_count_name = strdup(var_buf(fc));
        char *buf_max_name = strdup(var_buf(fc));
        char *buf_error_name = strdup(var_buf(fc));
        str_append_chars(fc->tkn_buffer, "unsigned long int ");
        str_append_chars(fc->tkn_buffer, buf_count_name);
        str_append_chars(fc->tkn_buffer, " = 0;\n");
        str_append_chars(fc->tkn_buffer, "unsigned long int ");
        str_append_chars(fc->tkn_buffer, buf_max_name);
        str_append_chars(fc->tkn_buffer, " = ");
        str_append_chars(fc->tkn_buffer, fcount->cname);
        str_append_chars(fc->tkn_buffer, "(");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ");\n");
        // While loop
        str_append_chars(fc->tkn_buffer, "while(");
        str_append_chars(fc->tkn_buffer, buf_count_name);
        str_append_chars(fc->tkn_buffer, " < ");
        str_append_chars(fc->tkn_buffer, buf_max_name);
        str_append_chars(fc->tkn_buffer, "){\n");
        // Get item
        str_append_chars(fc->tkn_buffer, "char* ");
        str_append_chars(fc->tkn_buffer, buf_error_name);
        str_append_chars(fc->tkn_buffer, " = (void*)0;");

        fc_write_c_type(fc->tkn_buffer, fget->return_type, te->vvar->gen_name);
        str_append_chars(fc->tkn_buffer, " = ");
        str_append_chars(fc->tkn_buffer, fget->cname);
        str_append_chars(fc->tkn_buffer, "(");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ",");
        str_append_chars(fc->tkn_buffer, buf_count_name);
        str_append_chars(fc->tkn_buffer, ", &");
        str_append_chars(fc->tkn_buffer, buf_error_name);
        str_append_chars(fc->tkn_buffer, ");\n");
        //
        if (te->kvar) {
            str_append_chars(fc->tkn_buffer, "unsigned long int ");
            str_append_chars(fc->tkn_buffer, te->kvar->gen_name);
            str_append_chars(fc->tkn_buffer, " = ");
            str_append_chars(fc->tkn_buffer, buf_count_name);
            str_append_chars(fc->tkn_buffer, ";\n");
        }
        // Increase index
        str_append_chars(fc->tkn_buffer, buf_count_name);
        str_append_chars(fc->tkn_buffer, "++;\n");
        // Check error
        str_append_chars(fc->tkn_buffer, "if(");
        str_append_chars(fc->tkn_buffer, buf_error_name);
        str_append_chars(fc->tkn_buffer, "){ continue; }\n");

        // Scope AST
        fc_write_c_ast(fc, te->scope);

        str_append_chars(fc->tkn_buffer, "}\n");

        free(buf_count_name);
        free(buf_max_name);
        free(buf_error_name);

    } else if (token->type == tkn_assign) {
        TokenAssign *ta = token->item;

        char *left;
        if (ta->left->type == vt_threaded_global) {
            // GlobalVar *gv = ta->left->item;
            left = strdup(var_buf(fc));
            fc_write_c_type(fc->tkn_buffer, ta->left->return_type, left);
            str_append_chars(fc->tkn_buffer, " = 0;\n");
        } else {
            fc_write_c_value(fc, ta->left, true, fc->tkn_buffer);
            left = str_to_chars(fc->value_buffer);
        }
        fc_write_c_value(fc, ta->right, true, fc->tkn_buffer);

        bool lrefc = false;
        bool lrefc_nullable = false;
        bool rrefc = false;
        bool rrefc_nullable = false;
        Class *class = NULL;
        if (ta->type == op_eq) {
            Value *left = ta->left;
            Value *right = ta->right;
            class = left->return_type->class;
            Class *rclass = right->return_type->class;

            if (class && class->ref_count) {
                lrefc = true;
                if (left->return_type->nullable) {
                    lrefc_nullable = true;
                }
            }
            if (rclass && rclass->ref_count) {
                rrefc = true;
                if (right->return_type->nullable) {
                    rrefc_nullable = true;
                }
            }
        }

        // RC++ the new value first
        if (rrefc) {
            if (rrefc_nullable) {
                str_append_chars(fc->tkn_buffer, "if(");
                str_append(fc->tkn_buffer, fc->value_buffer);
                str_append_chars(fc->tkn_buffer, "){ ");
            }
            str_append(fc->tkn_buffer, fc->value_buffer);
            str_append_chars(fc->tkn_buffer, "->_RC++;");
            if (rrefc_nullable) {
                str_append_chars(fc->tkn_buffer, " }");
            }
            str_append_chars(fc->tkn_buffer, "\n");
        }

        // RC-- the old value
        if (lrefc) {
            if (lrefc_nullable) {
                str_append_chars(fc->tkn_buffer, "if(");
                str_append_chars(fc->tkn_buffer, left);
                str_append_chars(fc->tkn_buffer, "){ ");
            }
            str_append_chars(fc->tkn_buffer, left);
            str_append_chars(fc->tkn_buffer, "->_RC--;\n");
            str_append_chars(fc->tkn_buffer, "if(");
            str_append_chars(fc->tkn_buffer, left);
            str_append_chars(fc->tkn_buffer, "->_RC == 0) ");
            str_append_chars(fc->tkn_buffer, class->cname);
            str_append_chars(fc->tkn_buffer, "____free(");
            str_append_chars(fc->tkn_buffer, left);
            str_append_chars(fc->tkn_buffer, ");");
            if (lrefc_nullable) {
                str_append_chars(fc->tkn_buffer, " }");
            }
            str_append_chars(fc->tkn_buffer, "\n");
        }

        str_append_chars(fc->tkn_buffer, left);

        if (ta->type == op_eq) {
            str_append_chars(fc->tkn_buffer, " = ");
        } else if (ta->type == op_add) {
            str_append_chars(fc->tkn_buffer, " += ");
        } else if (ta->type == op_sub) {
            str_append_chars(fc->tkn_buffer, " -= ");
        } else if (ta->type == op_mult) {
            str_append_chars(fc->tkn_buffer, " *= ");
        } else if (ta->type == op_div) {
            str_append_chars(fc->tkn_buffer, " /= ");
        } else if (ta->type == op_mod) {
            str_append_chars(fc->tkn_buffer, " \%= ");
        } else if (ta->type == op_bit_OR) {
            str_append_chars(fc->tkn_buffer, " |= ");
        } else if (ta->type == op_bit_AND) {
            str_append_chars(fc->tkn_buffer, " &= ");
        } else if (ta->type == op_bit_XOR) {
            str_append_chars(fc->tkn_buffer, " ^= ");
        } else {
            fc_error(fc, "Unhandled assign operator translation", NULL);
        }

        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ";\n");

        if (ta->left->type == vt_threaded_global) {
            GlobalVar *gv = ta->left->item;
            str_append_chars(fc->tkn_buffer, "pthread_setspecific(");
            str_append_chars(fc->tkn_buffer, gv->cname);
            str_append_chars(fc->tkn_buffer, ", ");
            str_append_chars(fc->tkn_buffer, left);
            str_append_chars(fc->tkn_buffer, ");\n");
        }

        free(left);

    } else if (token->type == tkn_set_vscope_value) {

        TokenSetVscopeValue *sv = token->item;
        fc_write_c_value(fc, sv->value, true, fc->tkn_buffer);

        deref_local_vars(fc, sv->value, sv->vscope);
        //
        str_append_chars(fc->tkn_buffer, sv->vname);
        str_append_chars(fc->tkn_buffer, " = ");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ";\n");

        str_append_chars(fc->tkn_buffer, "goto ");
        str_append_chars(fc->tkn_buffer, sv->vname);
        str_append_chars(fc->tkn_buffer, "_GOTO;\n");

    } else if (token->type == tkn_if) {
        fc_write_c_if(fc, token->item);
    } else if (token->type == tkn_ifnull) {
        //
        TokenIfNull *ifn = token->item;

        char *left = ifn->name;

        str_append_chars(fc->tkn_buffer, "if(");
        str_append_chars(fc->tkn_buffer, left);
        str_append_chars(fc->tkn_buffer, " == (void*)0) {\n");

        char *buf = fc_write_c_ort(fc, ifn->ort);

        if (buf) {
            str_append_chars(fc->tkn_buffer, left);
            str_append_chars(fc->tkn_buffer, " = ");
            str_append_chars(fc->tkn_buffer, buf);
            str_append_chars(fc->tkn_buffer, ";\n");
            free(buf);

            LocalVar *lv = ifn->idf->item;
            Type *type = lv->type;

            if (type->class && type->class->ref_count) {
                str_append_chars(fc->tkn_buffer, left);
                str_append_chars(fc->tkn_buffer, "->_RC++;\n");
            }
        }

        str_append_chars(fc->tkn_buffer, "}");

        if (ifn->ort->else_scope) {
            str_append_chars(fc->tkn_buffer, " else {\n");
            fc_write_c_ast(fc, ifn->ort->else_scope);
            str_append_chars(fc->tkn_buffer, "}");
        }

        str_append_chars(fc->tkn_buffer, "\n");

        //
    } else if (token->type == tkn_notnull) {
        //
        TokenNotNull *inn = token->item;
        str_append_chars(fc->tkn_buffer, "if(");
        str_append_chars(fc->tkn_buffer, inn->name);
        str_append_chars(fc->tkn_buffer, " != (void*)0) {\n");

        if (inn->type == or_do) {
            fc_write_c_ast(fc, inn->scope);
        }

        str_append_chars(fc->tkn_buffer, "}");

        if (inn->else_scope) {
            str_append_chars(fc->tkn_buffer, " else {\n");
            fc_write_c_ast(fc, inn->else_scope);
            str_append_chars(fc->tkn_buffer, "}");
        }

        str_append_chars(fc->tkn_buffer, "\n");

    } else if (token->type == tkn_while) {
        //
        TokenWhile *wt = token->item;
        fc_write_c_value(fc, wt->condition, true, fc->tkn_buffer);
        str_append_chars(fc->tkn_buffer, "while(");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ") {\n");
        fc_write_c_ast(fc, wt->scope);
        str_append_chars(fc->tkn_buffer, "}\n\n");
    } else if (token->type == tkn_break) {
        Scope *scope = fc->current_scope;
        scope = get_loop_scope(scope);
        deref_local_vars(fc, NULL, scope);
        str_append_chars(fc->tkn_buffer, "break;\n");
    } else if (token->type == tkn_continue) {
        Scope *scope = fc->current_scope;
        scope = get_loop_scope(scope);
        deref_local_vars(fc, NULL, scope);
        str_append_chars(fc->tkn_buffer, "continue;\n");
    } else if (token->type == tkn_throw) {
        TokenThrow *tt = token->item;
        str_append_chars(fc->tkn_buffer, "*_KI_THROW_MSG = \"");
        str_append_chars(fc->tkn_buffer, tt->msg);
        str_append_chars(fc->tkn_buffer, "\";\n");
        if (tt->return_type == NULL) {
            str_append_chars(fc->tkn_buffer, "return;\n");
        } else if (tt->return_type->is_pointer) {
            str_append_chars(fc->tkn_buffer, "return (void*)0;\n");
        } else {
            str_append_chars(fc->tkn_buffer, "return 0;\n");
        }

    } else if (token->type == tkn_free) {
        fc_write_c_value(fc, token->item, true, fc->tkn_buffer);
        str_append_chars(fc->tkn_buffer, "ki__mem__free(");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ");\n");
    } else if (token->type == tkn_value) {
        Value *val = token->item;
        fc_write_c_value(fc, val, true, fc->tkn_buffer);
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ";\n");
        */
    } else {
        printf("File: %s\n", fc->ki_filepath);
        printf("Token: %d\n", token->type);
        die("Unhandled token");
    }
}

/*
void fc_write_c_pre(FileCompiler *fc) {
    char *code;
    char *path = malloc(KI_PATH_MAX);
    strcpy(path, fc->x_filepath);
    strcat(path, "_start.h");

    if (fc->should_recompile) {
        code = str_to_chars(fc->h_code_start);
        write_file(path, code, false);
    } else {
        Str *content = file_get_contents(path);
        code = str_to_chars(content);
        free_str(content);
    }

    char *cache_dir = get_cache_dir();
    strcpy(path, cache_dir);
    strcat(path, "/project.h");
    write_file(path, code, true);

    free(code);
}

void fc_write_c_inits() {
    //
    char *cache_dir = get_cache_dir();
    char *hpath = malloc(KI_PATH_MAX);
    strcpy(hpath, cache_dir);
    strcat(hpath, "/project.h");
    //
    Str *all_code = str_make("");
    //
    str_append_chars(all_code, "#include \"project.h\"\n\n");
    str_append_chars(all_code, "void KI_INITS(){\n");

    // for (int i = 0; i < allocators->keys->length; i++) {
    //     char *name = array_get_index(allocators->values, i);
    //     if (name[strlen(name) - 1] == '0') {
    //         str_append_chars(all_code, name);
    //         str_append_chars(all_code, "__TK = (void*) 0;\n");
    //     } else {
    //         str_append_chars(all_code, "pthread_key_create(&");
    //         str_append_chars(all_code, name);
    //         str_append_chars(all_code, "__TK, (void*)0);\n");
    //     }
    // }

    for (int i = 0; i < packages->keys->length; i++) {
        PkgCompiler *pkc = array_get_index(packages->values, i);
        for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
            FileCompiler *fc = array_get_index(pkc->file_compilers->values, o);

            Str *code = str_make("");

            char *path = malloc(KI_PATH_MAX);
            strcpy(path, fc->x_filepath);
            strcat(path, "_init.c");

            if (fc->should_recompile) {
                //
                for (int x = 0; x < fc->globals->length; x++) {
                    GlobalVar *gv = array_get_index(fc->globals, x);

                    if (gv->type == gv_threaded) {
                        str_append_chars(code, "pthread_key_create(&");
                        str_append_chars(code, gv->cname);
                        str_append_chars(code, ", (void*)0);\n");
                    } else if (gv->type == gv_shared) {
                        fc_write_c_value(fc, gv->default_value, true, code);
                        str_append_chars(code, gv->cname);
                        str_append_chars(code, " = ");
                        str_append(code, fc->value_buffer);
                        str_append_chars(code, ";\n");
                    }
                }

                for (int x = 0; x < fc->strings->length; x++) {
                    ValueString *vstr = array_get_index(fc->strings, x);

                    char *gname = vstr->name;

                    str_append_chars(code, gname);
                    str_append_chars(code, " = ki__type__String__make(\"");
                    char *str = vstr->body;
                    str_append_chars(code, str);
                    str_append_chars(code, "\", ");
                    size_t len = strlen(str);
                    int count = 0;
                    int diff = 0;
                    char ch = '\0';
                    char pch = '\0';
                    // Dont count backslashes
                    while (count < len) {
                        pch = ch;
                        ch = str[count];
                        if (pch == '\\') {
                            diff++;
                            count++;
                            if (count < len) {
                                ch = str[count];
                            }
                        }
                        count++;
                    }

                    len -= diff;
                    char lenstr[20];
                    sprintf(lenstr, "%zu", len);
                    str_append_chars(code, lenstr);
                    str_append_chars(code, ", 1);\n");
                    // Keep in memory
                    str_append_chars(code, gname);
                    str_append_chars(code, "->_RC++;\n");
                }

                // Save
                if (code->length > 0) {
                    char *code_ = str_to_chars(code);
                    write_file(path, code_, false);
                    free(code_);
                } else {
                    if (file_exists(path)) {
                        remove(path);
                    }
                }
            } else {
                //
                if (file_exists(path)) {
                    Str *_code = file_get_contents(path);
                    str_append(code, _code);
                    free(_code);
                }
            }
            //
            free(path);

            //
            str_append_chars(all_code, "////////////////////////////////////////////\n");
            str_append_chars(all_code, "// from: ");
            str_append_chars(all_code, fc->ki_filepath);
            str_append_chars(all_code, "\n");
            str_append_chars(all_code, "////////////////////////////////////////////\n");
            str_append_chars(all_code, "\n");
            str_append(all_code, code);
            str_append_chars(all_code, "\n\n");

            free_str(code);
        }
    }

    str_append_chars(all_code, "}\n");

    str_append_chars(all_code, "void KI_INIT_THREAD(){\n");

    for (int i = 0; i < packages->keys->length; i++) {
        PkgCompiler *pkc = array_get_index(packages->values, i);
        for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
            FileCompiler *fc = array_get_index(pkc->file_compilers->values, o);

            Str *code = str_make("");

            char *path = malloc(KI_PATH_MAX);
            strcpy(path, fc->x_filepath);
            strcat(path, "_init_thread.c");

            if (fc->should_recompile) {
                //
                for (int x = 0; x < fc->globals->length; x++) {
                    GlobalVar *gv = array_get_index(fc->globals, x);

                    if (gv->type == gv_threaded) {
                        fc_write_c_value(fc, gv->default_value, true, code);

                        str_append_chars(code, "pthread_setspecific(");
                        str_append_chars(code, gv->cname);
                        str_append_chars(code, ", ");
                        str_append(code, fc->value_buffer);
                        str_append_chars(code, ");\n");
                    }
                }
            } else {
                //
                if (file_exists(path)) {
                    Str *_code = file_get_contents(path);
                    str_append(code, _code);
                    free(_code);
                }
            }
            //
            free(path);

            //
            str_append_chars(all_code, "////////////////////////////////////////////\n");
            str_append_chars(all_code, "// from: ");
            str_append_chars(all_code, fc->ki_filepath);
            str_append_chars(all_code, "\n");
            str_append_chars(all_code, "////////////////////////////////////////////\n");
            str_append_chars(all_code, "\n");
            str_append(all_code, code);
            str_append_chars(all_code, "\n\n");

            free_str(code);
        }
    }

    str_append_chars(all_code, "}\n");

    // Allocators
    Str *hcode = str_make("\n// Allocators\n\n");
    Map *dupes = map_make();
    for (int i = 0; i < packages->keys->length; i++) {
        PkgCompiler *pkc = array_get_index(packages->values, i);
        for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
            FileCompiler *fc = array_get_index(pkc->file_compilers->values, o);

            Map *alcs = fc->cache->allocators;
            for (int x = 0; x < alcs->keys->length; x++) {
                char *size = array_get_index(alcs->keys, x);
                char *threaded = array_get_index(alcs->values, x);

                sprintf(fc->sprintf, "KI_allocator_%s_%s", size, threaded);
                char *name = strdup(fc->sprintf);

                char *get = map_get(dupes, name);
                if (!get) {
                    fc_write_c_write_allocator(all_code, hcode, name, size, strcmp(threaded, "1") == 0);
                    map_set(dupes, name, name);
                }
            }
        }
    }

    // Main
    str_append_chars(all_code, "int main(");
    str_append_chars(all_code, "int _KI_ARGC, char** _KI_ARGV");
    str_append_chars(all_code, "){\n");

    str_append_chars(all_code, "KI_ALLOCATORS = ki__mem__calloc_flat(64 * 8);\n");
    str_append_chars(all_code, "KI_ALLOCATORS_MUT = ki__async__Mutex__make();\n");
    str_append_chars(all_code, "KI_INITS();\n");

    if (uses_async) {
        str_append_chars(all_code, "void* KI_MAIN_TMS = ki__async__Taskman__setup_task_managers();\n");
    }

    str_append_chars(all_code, "int result = 0;\n");

    if (g_run_tests) {
        str_append_chars(all_code, "ki__sys__tests_start();\n");
        for (int i = 0; i < g_test_funcs->length; i++) {
            Function *func = array_get_index(g_test_funcs, i);

            sprintf(g_sprintf, "ki__sys__test_start(\"%s\");\n", func->cname);
            str_append_chars(all_code, g_sprintf);

            str_append_chars(all_code, "result = ");
            str_append_chars(all_code, func->cname);
            str_append_chars(all_code, "();\n");
            str_append_chars(all_code, "ki__sys__test_result(result);\n");
        }
        str_append_chars(all_code, "ki__sys__tests_ready();\n");

    } else if (g_main_func) {
        Function *func = g_main_func;
        if (func->args->length == 1) {
            FunctionArg *arg = array_get_index(func->args, 0);
            Type *type = arg->type;
            Class *class = type->class;
            str_append_chars(all_code, "struct ");
            str_append_chars(all_code, class->cname);
            str_append_chars(all_code, "* ");
            str_append_chars(all_code, arg->name);
            str_append_chars(all_code, " = ki__type__String__generate_main_args(_KI_ARGC, _KI_ARGV);\n");
        }

        str_append_chars(all_code, "result = _KI_MAIN(\n");
        if (func->args->length == 1) {
            FunctionArg *arg = array_get_index(func->args, 0);
            str_append_chars(all_code, arg->name);
        }
        str_append_chars(all_code, ");\n");
    }

    if (uses_async) {
        str_append_chars(all_code, "ki__async__Taskman__wait_for_tasks_to_end(KI_MAIN_TMS);\n");
    }

    str_append_chars(all_code, "return result;\n");
    str_append_chars(all_code, "}\n\n");

    //
    char *hcode_ = str_to_chars(hcode);
    write_file(hpath, hcode_, true);
    free(hcode_);

    //
    char *code_ = str_to_chars(all_code);
    char *path = malloc(KI_PATH_MAX);
    strcpy(path, cache_dir);
    strcat(path, "/inits.c");
    write_file(path, code_, false);

    free(code_);
    free_str(all_code);
}

void fc_write_c(FileCompiler *fc) {

    if (!fc->should_recompile) {
        char *path = malloc(KI_PATH_MAX);
        char *cache_dir = get_cache_dir();
        strcpy(path, cache_dir);
        strcat(path, "/project.h");

        Str *h = file_get_contents(fc->h_filepath);
        char *hcode = str_to_chars(h);
        free_str(h);

        write_file(path, hcode, true);
        free(hcode);
        free(path);

        if (fc->is_used && file_exists(fc->o_filepath)) {
            array_push(o_files, fc->o_filepath);
        }
        return;
    }

    // Write c + o file
    char *hcode = str_to_chars(fc->h_code);
    char *code = str_to_chars(fc->c_code);
    char *code_gen = str_to_chars(fc->c_code_after);

    // printf("code:\n");
    // printf("%s\n", code);

    fc->create_o_file = false;
    if (strlen(code) > 0 || strlen(code_gen) > 0) {
        fc->create_o_file = true;
        if (true) {
            write_file(fc->c_filepath, "\n#include \"project.h\"\n\n", false);
            write_file(fc->c_filepath, "\n", true);
            if (!fc->is_header) {
                write_file(fc->c_filepath, code, true);
            }
            write_file(fc->c_filepath, code_gen, true);
        }

        if (fc->is_used) {
            array_push(o_files, fc->o_filepath);
        }
    }

    if (fc->is_header && false) {
        write_file(fc->h_filepath, hcode, false);
    } else {
        //
        write_file(fc->h_filepath, hcode, false);
        //
        char *path = malloc(KI_PATH_MAX);
        char *cache_dir = get_cache_dir();
        strcpy(path, cache_dir);
        strcat(path, "/project.h");
        write_file(path, hcode, true);
        free(path);
    }

    free(hcode);
    free(code);
    free(code_gen);
}

void fc_write_c_predefine_class(FileCompiler *fc, Class *class) {

    Str *code = fc->h_code_start;

    if (class->is_ctype) {
        str_append_chars(code, "typedef struct ");
        str_append_chars(code, class->cname);
        str_append_chars(code, " ");
        str_append_chars(code, class->cname);
        str_append_chars(code, ";\n");
    }

    str_append_chars(code, "struct ");
    str_append_chars(code, class->cname);
    str_append_chars(code, ";\n");
}

void fc_write_c_class(FileCompiler *fc, Class *class) {
    //
    str_append_chars(fc->h_code, "struct ");
    str_append_chars(fc->h_code, class->cname);
    str_append_chars(fc->h_code, " {\n");
    for (int i = 0; i < class->props->keys->length; i++) {
        char *name = array_get_index(class->props->keys, i);
        ClassProp *prop = array_get_index(class->props->values, i);
        if (prop->is_func) {
            continue;
        }
        if (prop->is_static) {
            continue;
        }
        fc_write_c_type(fc->h_code, prop->return_type, name);
        str_append_chars(fc->h_code, ";\n");
    }
    str_append_chars(fc->h_code, "};\n");

    // Free func
    ClassProp *prop = map_get(class->props, "__free");
    if (!prop || prop->generate_code == false) {

        str_append_chars(fc->h_code, "void ");
        str_append_chars(fc->h_code, class->cname);
        str_append_chars(fc->h_code, "____free(struct ");
        str_append_chars(fc->h_code, class->cname);
        str_append_chars(fc->h_code, "* this);\n");

        str_append_chars(fc->c_code, "void ");
        str_append_chars(fc->c_code, class->cname);
        str_append_chars(fc->c_code, "____free(struct ");
        str_append_chars(fc->c_code, class->cname);
        str_append_chars(fc->c_code, "* this){\n");

        ClassProp *before_prop = map_get(class->props, "__before_free");
        if (before_prop) {
            str_append_chars(fc->c_code, class->cname);
            str_append_chars(fc->c_code, "____before_free(this);\n");
        }

        for (int i = 0; i < class->props->keys->length; i++) {
            char *name = array_get_index(class->props->keys, i);
            ClassProp *prop = array_get_index(class->props->values, i);
            if (prop->is_func) {
                continue;
            }
            if (prop->is_static) {
                continue;
            }

            if (prop->return_type->class && prop->return_type->class->ref_count) {
                bool nullable = prop->return_type->nullable;
                if (nullable) {
                    str_append_chars(fc->c_code, "if(this->");
                    str_append_chars(fc->c_code, name);
                    str_append_chars(fc->c_code, ") {\n");
                }
                str_append_chars(fc->c_code, "if(--this->");
                str_append_chars(fc->c_code, name);
                str_append_chars(fc->c_code, "->_RC == 0) ");
                str_append_chars(fc->c_code, prop->return_type->class->cname);
                str_append_chars(fc->c_code, "____free(this->");
                str_append_chars(fc->c_code, name);
                str_append_chars(fc->c_code, ");\n");
                if (nullable) {
                    str_append_chars(fc->c_code, "}\n");
                }
            }
        }

        char *alloc_func = fc_write_c_get_allocator(fc, class->size, true);

        str_append_chars(fc->c_code, "ki__mem__Allocator__free(");
        str_append_chars(fc->c_code, alloc_func);
        str_append_chars(fc->c_code, "(), this);\n");
        str_append_chars(fc->c_code, "}\n\n");
    }
}

void fc_write_c_enum(FileCompiler *fc, Enum *enu) {
    //
    str_append_chars(fc->h_code, "typedef enum ");
    str_append_chars(fc->h_code, enu->cname);
    str_append_chars(fc->h_code, "{\n");

    int len = enu->values->keys->length;
    for (int i = 0; i < len; i++) {
        char *name = array_get_index(enu->values->keys, i);
        char *value = array_get_index(enu->values->values, i);
        str_append_chars(fc->h_code, name);
        str_append_chars(fc->h_code, " = ");
        str_append_chars(fc->h_code, value);
        str_append_chars(fc->h_code, ",\n");
    }

    str_append_chars(fc->h_code, "} ");
    str_append_chars(fc->h_code, enu->cname);
    str_append_chars(fc->h_code, ";\n");
}

void fc_write_c_global(FileCompiler *fc, GlobalVar *gv) {
    //
    if (gv->type == gv_threaded) {
        str_append_chars(fc->h_code, "struct pthread_key_t ");
        str_append_chars(fc->h_code, gv->cname);
        str_append_chars(fc->h_code, ";\n");
    } else if (gv->type == gv_shared) {
        fc_write_c_type(fc->h_code, gv->return_type, gv->cname);
        str_append_chars(fc->h_code, ";\n");
    }
}

void fc_write_c_func(FileCompiler *fc, Function *func) {

    //
    fc_write_c_type(fc->tkn_buffer, func->return_type, NULL);
    fc_write_c_type(fc->h_code, func->return_type, NULL);
    str_append_chars(fc->tkn_buffer, " ");
    str_append_chars(fc->h_code, " ");
    if (strcmp(func->cname, "main") == 0) {
        str_append_chars(fc->tkn_buffer, "_KI_MAIN");
        str_append_chars(fc->h_code, "_KI_MAIN");
    } else {
        str_append_chars(fc->tkn_buffer, func->cname);
        str_append_chars(fc->h_code, func->cname);
    }
    str_append_chars(fc->tkn_buffer, "(");
    str_append_chars(fc->h_code, "(");
    // Write args
    int arg_len = func->args->length;
    int x = 0;
    while (x < arg_len) {
        FunctionArg *arg = array_get_index(func->args, x);
        char *name = arg->name;
        Type *type = arg->type;
        fc_write_c_type(fc->tkn_buffer, type, name);
        fc_write_c_type(fc->h_code, type, name);
        x++;
        if (x < arg_len) {
            str_append_chars(fc->tkn_buffer, ", ");
            str_append_chars(fc->h_code, ", ");
        }
    }

    if (func->can_error) {
        if (arg_len > 0) {
            str_append_chars(fc->tkn_buffer, ", ");
            str_append_chars(fc->h_code, ", ");
        }
        str_append_chars(fc->tkn_buffer, "char** _KI_THROW_MSG");
        str_append_chars(fc->h_code, "char** _KI_THROW_MSG");
    }
    //
    str_append_chars(fc->h_code, ");\n");

    if (!fc->is_header) {
        str_append_chars(fc->tkn_buffer, ") {\n");
        fc->indent++;

        if (func->scope->catch_errors) {
            str_append_chars(fc->tkn_buffer, "char* _KI_THROW_MSG_BUF = 0;\n");
        }

        // If we want to assign new values to argument variables, this code is required
        // Currently we dont allow this, because of performance (extra ref counting)
        // int x = 0;
        // while (x < arg_len) {
        //     FunctionArg *arg = array_get_index(func->args, x);
        //     char *name = arg->name;
        //     Type *type = arg->type;
        //     x++;
        //     Class *class = type->class;
        //     if (class && class->ref_count) {
        //         if (type->nullable) {
        //             str_append_chars(fc->tkn_buffer, "if(");
        //             str_append_chars(fc->tkn_buffer, name);
        //             str_append_chars(fc->tkn_buffer, ") ");
        //         }
        //         str_append_chars(fc->tkn_buffer, name);
        //         str_append_chars(fc->tkn_buffer, "->_RC++;\n");
        //     }
        // }

        // Body
        fc_write_c_ast(fc, func->scope);

        fc->indent--;
        str_append_chars(fc->tkn_buffer, "}\n\n");
    }
}

void fc_write_c_ast(FileCompiler *fc, Scope *scope) {
    Scope *prev_scope = fc->current_scope;
    fc->current_scope = scope;

    int c = 0;
    Array *ast = scope->ast;
    while (c < ast->length) {
        Token *t = array_get_index(ast, c);
        fc_write_c_token(fc, t);
        c++;
    }

    if (!scope->did_return) {
        deref_local_vars(fc, NULL, fc->current_scope);
    }

    fc->current_scope = prev_scope;
}

void fc_write_c_token(FileCompiler *fc, Token *token) {
    Str *prev_buf = fc->tkn_buffer;
    Str *prev_before_buf = fc->before_tkn_buffer;
    Str *buf = str_make("");
    Str *before_buf = str_make("");
    fc->tkn_buffer = buf;
    fc->before_tkn_buffer = before_buf;
    //
    // printf("tt:%d\n", token->type);
    fc_indent(fc, fc->tkn_buffer);
    if (token->type == tkn_func) {
        fc_write_c_func(fc, token->item);
    } else if (token->type == tkn_init_thread) {
        str_append_chars(fc->tkn_buffer, "KI_INIT_THREAD();\n");
    } else if (token->type == tkn_debug_msg) {
        char *msg = token->item;
        str_append_chars(fc->tkn_buffer, "write(1, \"");
        str_append_chars(fc->tkn_buffer, msg);
        str_append_chars(fc->tkn_buffer, "\\n\", ");
        sprintf(fc->sprintf, "%ld", strlen(msg) + 1);
        str_append_chars(fc->tkn_buffer, fc->sprintf);
        str_append_chars(fc->tkn_buffer, ");\n");
        // TODO: Delete errno stuff
        str_append_chars(fc->tkn_buffer, "write(1, \"errno:\", 6);\n");
        str_append_chars(fc->tkn_buffer, "struct ki__type__String* ERRNOMSG = ki__type__i32__str(errno);\n");
        str_append_chars(fc->tkn_buffer, "write(1, ERRNOMSG->data, ERRNOMSG->bytes);\n");
        str_append_chars(fc->tkn_buffer, "write(1, \"\\n\", 1);\n");
    } else if (token->type == tkn_exit) {
        ErrorToken *err = token->item;
        str_append_chars(fc->tkn_buffer, "ki__sys__exit(");
        str_append_chars(fc->tkn_buffer, err->msg);
        str_append_chars(fc->tkn_buffer, ");\n");
    } else if (token->type == tkn_panic) {
        ErrorToken *err = token->item;
        str_append_chars(fc->tkn_buffer, "write(1, \"");
        str_append_chars(fc->tkn_buffer, err->msg);
        str_append_chars(fc->tkn_buffer, "\", ");
        sprintf(fc->sprintf, "%ld", strlen(err->msg));
        str_append_chars(fc->tkn_buffer, fc->sprintf);
        str_append_chars(fc->tkn_buffer, ");\n");
        str_append_chars(fc->tkn_buffer, "ki__sys__exit(1);\n");
    } else if (token->type == tkn_each) {
        TokenEach *te = token->item;
        fc_write_c_value(fc, te->value, true, fc->tkn_buffer);
        Class *class = te->value->return_type->class;
        ClassProp *fcountp = map_get(class->props, "__each_count");
        ClassProp *fgetp = map_get(class->props, "__each_get");
        Function *fcount = fcountp->func;
        Function *fget = fgetp->func;

        char *buf_count_name = strdup(var_buf(fc));
        char *buf_max_name = strdup(var_buf(fc));
        char *buf_error_name = strdup(var_buf(fc));
        str_append_chars(fc->tkn_buffer, "unsigned long int ");
        str_append_chars(fc->tkn_buffer, buf_count_name);
        str_append_chars(fc->tkn_buffer, " = 0;\n");
        str_append_chars(fc->tkn_buffer, "unsigned long int ");
        str_append_chars(fc->tkn_buffer, buf_max_name);
        str_append_chars(fc->tkn_buffer, " = ");
        str_append_chars(fc->tkn_buffer, fcount->cname);
        str_append_chars(fc->tkn_buffer, "(");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ");\n");
        // While loop
        str_append_chars(fc->tkn_buffer, "while(");
        str_append_chars(fc->tkn_buffer, buf_count_name);
        str_append_chars(fc->tkn_buffer, " < ");
        str_append_chars(fc->tkn_buffer, buf_max_name);
        str_append_chars(fc->tkn_buffer, "){\n");
        // Get item
        str_append_chars(fc->tkn_buffer, "char* ");
        str_append_chars(fc->tkn_buffer, buf_error_name);
        str_append_chars(fc->tkn_buffer, " = (void*)0;");

        fc_write_c_type(fc->tkn_buffer, fget->return_type, te->vvar->gen_name);
        str_append_chars(fc->tkn_buffer, " = ");
        str_append_chars(fc->tkn_buffer, fget->cname);
        str_append_chars(fc->tkn_buffer, "(");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ",");
        str_append_chars(fc->tkn_buffer, buf_count_name);
        str_append_chars(fc->tkn_buffer, ", &");
        str_append_chars(fc->tkn_buffer, buf_error_name);
        str_append_chars(fc->tkn_buffer, ");\n");
        //
        if (te->kvar) {
            str_append_chars(fc->tkn_buffer, "unsigned long int ");
            str_append_chars(fc->tkn_buffer, te->kvar->gen_name);
            str_append_chars(fc->tkn_buffer, " = ");
            str_append_chars(fc->tkn_buffer, buf_count_name);
            str_append_chars(fc->tkn_buffer, ";\n");
        }
        // Increase index
        str_append_chars(fc->tkn_buffer, buf_count_name);
        str_append_chars(fc->tkn_buffer, "++;\n");
        // Check error
        str_append_chars(fc->tkn_buffer, "if(");
        str_append_chars(fc->tkn_buffer, buf_error_name);
        str_append_chars(fc->tkn_buffer, "){ continue; }\n");

        // Scope AST
        fc_write_c_ast(fc, te->scope);

        str_append_chars(fc->tkn_buffer, "}\n");

        free(buf_count_name);
        free(buf_max_name);
        free(buf_error_name);

    } else if (token->type == tkn_declare) {
        TokenDeclare *decl = token->item;

        fc_write_c_value(fc, decl->value, true, fc->tkn_buffer);

        fc_write_c_type(fc->tkn_buffer, decl->type, decl->name);
        str_append_chars(fc->tkn_buffer, " = ");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ";\n");

        Class *class = decl->value->return_type->class;
        bool nullable = decl->value->return_type->nullable;
        if (class && class->ref_count) {
            if (nullable) {
                str_append_chars(fc->tkn_buffer, "if(");
                str_append_chars(fc->tkn_buffer, decl->name);
                str_append_chars(fc->tkn_buffer, ") ");
            }
            str_append_chars(fc->tkn_buffer, decl->name);
            str_append_chars(fc->tkn_buffer, "->_RC++;\n");

            array_push(fc->current_scope->local_var_names, decl);
        }
    } else if (token->type == tkn_assign) {
        TokenAssign *ta = token->item;

        char *left;
        if (ta->left->type == vt_threaded_global) {
            // GlobalVar *gv = ta->left->item;
            left = strdup(var_buf(fc));
            fc_write_c_type(fc->tkn_buffer, ta->left->return_type, left);
            str_append_chars(fc->tkn_buffer, " = 0;\n");
        } else {
            fc_write_c_value(fc, ta->left, true, fc->tkn_buffer);
            left = str_to_chars(fc->value_buffer);
        }
        fc_write_c_value(fc, ta->right, true, fc->tkn_buffer);

        bool lrefc = false;
        bool lrefc_nullable = false;
        bool rrefc = false;
        bool rrefc_nullable = false;
        Class *class = NULL;
        if (ta->type == op_eq) {
            Value *left = ta->left;
            Value *right = ta->right;
            class = left->return_type->class;
            Class *rclass = right->return_type->class;

            if (class && class->ref_count) {
                lrefc = true;
                if (left->return_type->nullable) {
                    lrefc_nullable = true;
                }
            }
            if (rclass && rclass->ref_count) {
                rrefc = true;
                if (right->return_type->nullable) {
                    rrefc_nullable = true;
                }
            }
        }

        // RC++ the new value first
        if (rrefc) {
            if (rrefc_nullable) {
                str_append_chars(fc->tkn_buffer, "if(");
                str_append(fc->tkn_buffer, fc->value_buffer);
                str_append_chars(fc->tkn_buffer, "){ ");
            }
            str_append(fc->tkn_buffer, fc->value_buffer);
            str_append_chars(fc->tkn_buffer, "->_RC++;");
            if (rrefc_nullable) {
                str_append_chars(fc->tkn_buffer, " }");
            }
            str_append_chars(fc->tkn_buffer, "\n");
        }

        // RC-- the old value
        if (lrefc) {
            if (lrefc_nullable) {
                str_append_chars(fc->tkn_buffer, "if(");
                str_append_chars(fc->tkn_buffer, left);
                str_append_chars(fc->tkn_buffer, "){ ");
            }
            str_append_chars(fc->tkn_buffer, left);
            str_append_chars(fc->tkn_buffer, "->_RC--;\n");
            str_append_chars(fc->tkn_buffer, "if(");
            str_append_chars(fc->tkn_buffer, left);
            str_append_chars(fc->tkn_buffer, "->_RC == 0) ");
            str_append_chars(fc->tkn_buffer, class->cname);
            str_append_chars(fc->tkn_buffer, "____free(");
            str_append_chars(fc->tkn_buffer, left);
            str_append_chars(fc->tkn_buffer, ");");
            if (lrefc_nullable) {
                str_append_chars(fc->tkn_buffer, " }");
            }
            str_append_chars(fc->tkn_buffer, "\n");
        }

        str_append_chars(fc->tkn_buffer, left);

        if (ta->type == op_eq) {
            str_append_chars(fc->tkn_buffer, " = ");
        } else if (ta->type == op_add) {
            str_append_chars(fc->tkn_buffer, " += ");
        } else if (ta->type == op_sub) {
            str_append_chars(fc->tkn_buffer, " -= ");
        } else if (ta->type == op_mult) {
            str_append_chars(fc->tkn_buffer, " *= ");
        } else if (ta->type == op_div) {
            str_append_chars(fc->tkn_buffer, " /= ");
        } else if (ta->type == op_mod) {
            str_append_chars(fc->tkn_buffer, " \%= ");
        } else if (ta->type == op_bit_OR) {
            str_append_chars(fc->tkn_buffer, " |= ");
        } else if (ta->type == op_bit_AND) {
            str_append_chars(fc->tkn_buffer, " &= ");
        } else if (ta->type == op_bit_XOR) {
            str_append_chars(fc->tkn_buffer, " ^= ");
        } else {
            fc_error(fc, "Unhandled assign operator translation", NULL);
        }

        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ";\n");

        if (ta->left->type == vt_threaded_global) {
            GlobalVar *gv = ta->left->item;
            str_append_chars(fc->tkn_buffer, "pthread_setspecific(");
            str_append_chars(fc->tkn_buffer, gv->cname);
            str_append_chars(fc->tkn_buffer, ", ");
            str_append_chars(fc->tkn_buffer, left);
            str_append_chars(fc->tkn_buffer, ");\n");
        }

        free(left);

    } else if (token->type == tkn_set_vscope_value) {

        TokenSetVscopeValue *sv = token->item;
        fc_write_c_value(fc, sv->value, true, fc->tkn_buffer);

        deref_local_vars(fc, sv->value, sv->vscope);
        //
        str_append_chars(fc->tkn_buffer, sv->vname);
        str_append_chars(fc->tkn_buffer, " = ");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ";\n");

        str_append_chars(fc->tkn_buffer, "goto ");
        str_append_chars(fc->tkn_buffer, sv->vname);
        str_append_chars(fc->tkn_buffer, "_GOTO;\n");

    } else if (token->type == tkn_return) {
        Value *retv = NULL;
        if (token->item) {
            retv = token->item;
            fc_write_c_value(fc, token->item, true, fc->tkn_buffer);
        }

        // Deref local vars + Check if var_bufs RC == 0 (if so free)
        deref_local_vars(fc, retv, NULL);

        //
        str_append_chars(fc->tkn_buffer, "return ");
        if (retv) {
            str_append(fc->tkn_buffer, fc->value_buffer);
        }
        str_append_chars(fc->tkn_buffer, ";\n");
    } else if (token->type == tkn_if) {
        fc_write_c_if(fc, token->item);
    } else if (token->type == tkn_ifnull) {
        //
        TokenIfNull *ifn = token->item;

        char *left = ifn->name;

        str_append_chars(fc->tkn_buffer, "if(");
        str_append_chars(fc->tkn_buffer, left);
        str_append_chars(fc->tkn_buffer, " == (void*)0) {\n");

        char *buf = fc_write_c_ort(fc, ifn->ort);

        if (buf) {
            str_append_chars(fc->tkn_buffer, left);
            str_append_chars(fc->tkn_buffer, " = ");
            str_append_chars(fc->tkn_buffer, buf);
            str_append_chars(fc->tkn_buffer, ";\n");
            free(buf);

            LocalVar *lv = ifn->idf->item;
            Type *type = lv->type;

            if (type->class && type->class->ref_count) {
                str_append_chars(fc->tkn_buffer, left);
                str_append_chars(fc->tkn_buffer, "->_RC++;\n");
            }
        }

        str_append_chars(fc->tkn_buffer, "}");

        if (ifn->ort->else_scope) {
            str_append_chars(fc->tkn_buffer, " else {\n");
            fc_write_c_ast(fc, ifn->ort->else_scope);
            str_append_chars(fc->tkn_buffer, "}");
        }

        str_append_chars(fc->tkn_buffer, "\n");

        //
    } else if (token->type == tkn_notnull) {
        //
        TokenNotNull *inn = token->item;
        str_append_chars(fc->tkn_buffer, "if(");
        str_append_chars(fc->tkn_buffer, inn->name);
        str_append_chars(fc->tkn_buffer, " != (void*)0) {\n");

        if (inn->type == or_do) {
            fc_write_c_ast(fc, inn->scope);
        }

        str_append_chars(fc->tkn_buffer, "}");

        if (inn->else_scope) {
            str_append_chars(fc->tkn_buffer, " else {\n");
            fc_write_c_ast(fc, inn->else_scope);
            str_append_chars(fc->tkn_buffer, "}");
        }

        str_append_chars(fc->tkn_buffer, "\n");

    } else if (token->type == tkn_while) {
        //
        TokenWhile *wt = token->item;
        fc_write_c_value(fc, wt->condition, true, fc->tkn_buffer);
        str_append_chars(fc->tkn_buffer, "while(");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ") {\n");
        fc_write_c_ast(fc, wt->scope);
        str_append_chars(fc->tkn_buffer, "}\n\n");
    } else if (token->type == tkn_break) {
        Scope *scope = fc->current_scope;
        scope = get_loop_scope(scope);
        deref_local_vars(fc, NULL, scope);
        str_append_chars(fc->tkn_buffer, "break;\n");
    } else if (token->type == tkn_continue) {
        Scope *scope = fc->current_scope;
        scope = get_loop_scope(scope);
        deref_local_vars(fc, NULL, scope);
        str_append_chars(fc->tkn_buffer, "continue;\n");
    } else if (token->type == tkn_throw) {
        TokenThrow *tt = token->item;
        str_append_chars(fc->tkn_buffer, "*_KI_THROW_MSG = \"");
        str_append_chars(fc->tkn_buffer, tt->msg);
        str_append_chars(fc->tkn_buffer, "\";\n");
        if (tt->return_type == NULL) {
            str_append_chars(fc->tkn_buffer, "return;\n");
        } else if (tt->return_type->is_pointer) {
            str_append_chars(fc->tkn_buffer, "return (void*)0;\n");
        } else {
            str_append_chars(fc->tkn_buffer, "return 0;\n");
        }

    } else if (token->type == tkn_free) {
        fc_write_c_value(fc, token->item, true, fc->tkn_buffer);
        str_append_chars(fc->tkn_buffer, "ki__mem__free(");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ");\n");
    } else if (token->type == tkn_value) {
        Value *val = token->item;
        fc_write_c_value(fc, val, true, fc->tkn_buffer);
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ";\n");
    } else {
        printf("Token: %d\n", token->type);
        fc_error(fc, "Unhandled token", NULL);
    }

    //
    str_append(before_buf, buf);
    free_str(buf);
    if (prev_buf == NULL) {
        str_append(fc->c_code, before_buf);
    } else {
        str_append(prev_buf, before_buf);
    }
    free_str(before_buf);
    //
    fc->tkn_buffer = prev_buf;
    fc->before_tkn_buffer = prev_before_buf;
}

char *fc_write_c_ort(FileCompiler *fc, OrToken *ort) {
    //
    char *buf = NULL;
    if (ort->type == or_value) {
        buf = strdup(var_buf(fc));
        fc_write_c_type(fc->tkn_buffer, ort->primary_type, buf);
        str_append_chars(fc->tkn_buffer, ";\n");
    }

    if (ort->type == or_value) {
        if (ort->vscope) {
            fc_write_c_type(fc->tkn_buffer, ort->vscope->vscope_return_type, ort->vscope->vscope_vname);
            str_append_chars(fc->tkn_buffer, ";\n");
        }
    }

    str_append_chars(fc->tkn_buffer, "if(1){\n");

    if (ort->type == or_value) {
        if (ort->vscope) {
            fc_write_c_ast(fc, ort->vscope);
        } else {
            fc_write_c_value(fc, ort->value, true, fc->tkn_buffer);
            str_append_chars(fc->tkn_buffer, buf);
            str_append_chars(fc->tkn_buffer, " = ");
            str_append(fc->tkn_buffer, fc->value_buffer);
            str_append_chars(fc->tkn_buffer, ";\n");
        }
    } else if (ort->type == or_return) {

        if (ort->vscope) {
            fc_write_c_ast(fc, ort->vscope);
        } else {

            Scope *fscope = get_func_scope(fc->current_scope);
            Scope *sub_scope = init_sub_scope(fc->current_scope);
            Scope *prev_scope = fc->current_scope;
            fc->current_scope = sub_scope;

            if (ort->value) {
                fc_write_c_value(fc, ort->value, true, fc->tkn_buffer);
            }

            deref_local_vars(fc, ort->value, fscope);
            fc->current_scope = prev_scope;
            free_scope(sub_scope);

            str_append_chars(fc->tkn_buffer, "return ");
            if (ort->value) {
                str_append(fc->tkn_buffer, fc->value_buffer);
            }
            str_append_chars(fc->tkn_buffer, ";\n");
        }
    } else if (ort->type == or_exit) {
        str_append_chars(fc->tkn_buffer, "exit(1);\n");
    } else if (ort->type == or_panic) {
        str_append_chars(fc->tkn_buffer, "exit(1);\n");
    } else if (ort->type == or_pass) {
        str_append_chars(fc->tkn_buffer, "*_KI_THROW_MSG = _KI_THROW_MSG_BUF;\n");
        str_append_chars(fc->tkn_buffer, "_KI_THROW_MSG_BUF = (void*)0;\n");
        Type *rett = ort->primary_type;
        if (rett == NULL) {
            str_append_chars(fc->tkn_buffer, "return;\n");
        } else if (rett->is_pointer) {
            str_append_chars(fc->tkn_buffer, "return (void*)0;\n");
        } else {
            str_append_chars(fc->tkn_buffer, "return 0;\n");
        }
    } else if (ort->type == or_throw) {
        str_append_chars(fc->tkn_buffer, "*_KI_THROW_MSG = \"");
        str_append_chars(fc->tkn_buffer, ort->error->msg);
        str_append_chars(fc->tkn_buffer, "\";\n");
        Scope *scope = fc->current_scope;
        scope = get_func_scope(scope);
        if (scope->func->return_type == NULL) {
            str_append_chars(fc->tkn_buffer, "return;\n");
        } else if (scope->func->return_type->is_pointer) {
            str_append_chars(fc->tkn_buffer, "return (void*)0;\n");
        } else {
            str_append_chars(fc->tkn_buffer, "return 0;\n");
        }
    } else if (ort->type == or_break) {
        Scope *scope = get_loop_scope(fc->current_scope);
        deref_local_vars(fc, NULL, scope);
        str_append_chars(fc->tkn_buffer, "break;\n");
    } else if (ort->type == or_continue) {
        Scope *scope = get_loop_scope(fc->current_scope);
        deref_local_vars(fc, NULL, scope);
        str_append_chars(fc->tkn_buffer, "continue;\n");
    } else if (ort->type == or_do) {
        fc_write_c_ast(fc, ort->vscope);
    }

    str_append_chars(fc->tkn_buffer, " }\n");

    if (ort->type == or_value) {
        if (ort->vscope) {
            str_append_chars(fc->tkn_buffer, ort->vscope->vscope_vname);
            str_append_chars(fc->tkn_buffer, "_GOTO:\n");

            str_append_chars(fc->tkn_buffer, buf);
            str_append_chars(fc->tkn_buffer, " = ");
            str_append_chars(fc->tkn_buffer, ort->vscope->vscope_vname);
            str_append_chars(fc->tkn_buffer, ";\n");
        }
    }

    return buf;
}

char *indenter;
void fc_indent(FileCompiler *fc, Str *append_to) {
    //
    int chars = fc->indent * 2;
    if (chars > 0) {
        if (indenter == NULL) {
            int max_size = 1000;
            indenter = malloc(max_size);
            int c = 0;
            while (c < max_size) {
                indenter[c] = ' ';
                c++;
            }
            indenter[max_size - 1] = '\0';
        }
        indenter[chars] = '\0';
        str_append_chars(append_to, indenter);
        indenter[chars] = ' ';
    }
}

void fc_write_c_value(FileCompiler *fc, Value *value, bool new_value, Str *code) {
    Str *result = fc->value_buffer;

    if (new_value) {
        result->length = 0;
    }

    //
    // printf("Type: %d\n", value->type);

}

char i_to_str_buf[100];

void fc_write_c_type_varname(Str *append_to, Type *type, char *varname) {
    if (varname) {
        if (type && type->type != type_funcref) {
            str_append_chars(append_to, " ");
        }
        str_append_chars(append_to, varname);
    }
    if (type && type->is_array) {
        str_append_chars(append_to, "[");
        sprintf(i_to_str_buf, "%d", type->array_size);
        str_append_chars(append_to, i_to_str_buf);
        str_append_chars(append_to, "]");
    }
}

void fc_write_c_type(Str *append_to, Type *type, char *varname) {
    if (type == NULL) {
        str_append_chars(append_to, "void");
        fc_write_c_type_varname(append_to, type, varname);
        return;
    }
    //
    if (type->type == type_bool) {
        str_append_chars(append_to, "unsigned char");
        if (type->is_pointer_of_pointer) {
            str_append_chars(append_to, "*");
        }
        fc_write_c_type_varname(append_to, type, varname);
        return;
    }
    //
    if (type->type == type_void_pointer) {
        str_append_chars(append_to, "void*");
        if (type->is_pointer_of_pointer) {
            str_append_chars(append_to, "*");
        }
        fc_write_c_type_varname(append_to, type, varname);
        return;
    }
    //
    if (type->type == type_funcref) {
        // {ret_type} (*{varname})({arg1_type}, {arg2_type}) = ...
        fc_write_c_type(append_to, type->func_return_type, NULL);
        str_append_chars(append_to, "(*");
        if (type->is_pointer_of_pointer) {
            str_append_chars(append_to, "*");
        }
        fc_write_c_type_varname(append_to, type, varname);
        str_append_chars(append_to, ")(");
        for (int i = 0; i < type->func_arg_types->length; i++) {
            Type *arg_type = array_get_index(type->func_arg_types, i);
            if (i > 0) {
                str_append_chars(append_to, ", ");
            }
            fc_write_c_type(append_to, arg_type, NULL);
        }
        str_append_chars(append_to, ")");
        return;
    }
    //
    if (type->type == type_struct) {
        Class *class = type->class;
        if (class->is_number) {
            int bytes = type->bytes;
            if (class->is_unsigned) {
                str_append_chars(append_to, "unsigned ");
            }
            while (bytes > 4) {
                bytes /= 2;
                str_append_chars(append_to, "long ");
            }
            if (bytes == 1) {
                str_append_chars(append_to, "char");
            } else if (bytes == 2) {
                str_append_chars(append_to, "short");
            } else if (bytes == 4) {
                str_append_chars(append_to, "int");
            } else {
                die("Cant determine c number type based on the amount of bytes");
            }
        } else {
            // Normal class
            if (!class->is_ctype) {
                str_append_chars(append_to, "struct ");
            }
            str_append_chars(append_to, class->cname);
        }

        if (type->is_pointer) {
            str_append_chars(append_to, "*");
        }
        if (type->is_pointer_of_pointer && type->type != type_funcref) {
            str_append_chars(append_to, "*");
        }
        fc_write_c_type_varname(append_to, type, varname);
        return;
    }
    if (type->type == type_enum) {
        str_append_chars(append_to, "int");
        fc_write_c_type_varname(append_to, type, varname);
        return;
    }
    if (type->type == type_null) {
        str_append_chars(append_to, "void");
        fc_write_c_type_varname(append_to, type, varname);
        return;
    }
    if (type->type == type_void) {
        str_append_chars(append_to, "void");
        fc_write_c_type_varname(append_to, type, varname);
        return;
    }
    printf("Could not convert type to c: %d\n", type->type);
    raise(SIGSEGV); // Useful for debugging
}

void fc_write_c_if(FileCompiler *fc, TokenIf *ift) {
    //
    if (ift->is_else) {
        str_append_chars(fc->tkn_buffer, " else {\n");
    }
    if (ift->condition) {
        fc_write_c_value(fc, ift->condition, true, fc->tkn_buffer);
        str_append_chars(fc->tkn_buffer, "if (");
        str_append(fc->tkn_buffer, fc->value_buffer);
        str_append_chars(fc->tkn_buffer, ") {\n");
    }

    fc_write_c_ast(fc, ift->scope);

    if (ift->condition) {
        str_append_chars(fc->tkn_buffer, "}\n");
    }

    if (ift->next) {
        fc_write_c_if(fc, ift->next);
    }

    if (ift->is_else) {
        str_append_chars(fc->tkn_buffer, "}\n");
    }
}

Str *value_buf(FileCompiler *fc) {
    fc->value_buffer->length = 0;
    return fc->value_buffer;
}
*/

char *var_buf(FileCompiler *fc) {
    strcpy(fc->var_buf, "_KI_VBUF");
    fc->var_bufc++;
    sprintf(fc->sprintf, "%d", fc->var_bufc);
    strcat(fc->var_buf, fc->sprintf);
    return fc->var_buf;
}

void llvm_deref_local_vars(FileCompiler *fc, Value *retv, Scope *until_scope) {
    //
    char *ignore_vbuf = NULL;
    if (retv && retv->return_type->class && retv->return_type->class->ref_count) {
        ignore_vbuf = str_to_chars(fc->value_buffer);
    }
    //
    Scope *scope = fc->current_scope;

    // If we want to assign new values to argument variables, this code is required
    // Currently we dont allow this, because of performance (extra ref counting)
    // Scope *func_scope = scope;
    // while (func_scope && !func_scope->func) {
    //     func_scope = func_scope->parent;
    // }

    // Write + Clear var bufs
    int c = 0;
    while (true) {
        c++;

        for (int i = 0; i < scope->var_bufs->length; i++) {
            VarInfo *vi = array_get_index(scope->var_bufs, i);
            char *vb = vi->name;
            Type *rt = vi->return_type;

            llvm_deref(fc, vi->llvm_value, rt);

            if (c == 1) {
                free(vb);
                free(vi);
            }
        }

        if (c == 1) {
            scope->var_bufs->length = 0;
        }

        // Clear local vars
        Array *local_vars = scope->local_var_names;
        for (int i = 0; i < local_vars->length; i++) {
            TokenDeclare *decl = array_get_index(local_vars, i);
            Class *class = decl->type->class;
            bool nullable = decl->type->nullable;
            char *lv = decl->name;

            if (nullable) {
                str_append_chars(fc->tkn_buffer, "if(");
                str_append_chars(fc->tkn_buffer, lv);
                str_append_chars(fc->tkn_buffer, "){ ");
            }
            str_append_chars(fc->tkn_buffer, "if(--");
            str_append_chars(fc->tkn_buffer, lv);
            str_append_chars(fc->tkn_buffer, "->_RC == 0) ");

            if (ignore_vbuf && strcmp(ignore_vbuf, lv) == 0) {
                str_append_chars(fc->tkn_buffer, "{}");
            } else {
                str_append_chars(fc->tkn_buffer, class->cname);
                str_append_chars(fc->tkn_buffer, "____free(");
                str_append_chars(fc->tkn_buffer, lv);
                str_append_chars(fc->tkn_buffer, ");");
            }

            if (nullable) {
                str_append_chars(fc->tkn_buffer, " }");
            }
            str_append_chars(fc->tkn_buffer, "\n");
        }

        if (scope->is_func) {
            break;
        }
        if (until_scope && scope == until_scope) {
            break;
        }

        scope = scope->parent;

        if (scope == NULL) {
            break;
        }
    }

    if (ignore_vbuf) {
        free(ignore_vbuf);
    }

    // If we want to assign new values to argument variables, this code is required
    // Currently we dont allow this, because of performance (extra ref counting)
    // // Deref func args
    // if (!once || scope->func) {
    //     if (func_scope) {
    //         Function *func = func_scope->func;
    //         int arg_len = func->args->length;
    //         int x = 0;
    //         while (x < arg_len) {
    //             FunctionArg *arg = array_get_index(func->args, x);
    //             char *name = arg->name;
    //             Type *type = arg->type;
    //             x++;
    //             Class *class = type->class;
    //             if (class && class->ref_count) {
    //                 if (type->nullable) {
    //                     str_append_chars(fc->tkn_buffer, "if(");
    //                     str_append_chars(fc->tkn_buffer, name);
    //                     str_append_chars(fc->tkn_buffer, ") ");
    //                 }
    //                 str_append_chars(fc->tkn_buffer, name);
    //                 str_append_chars(fc->tkn_buffer, "->_RC--;\n");
    //             }
    //         }
    //     }
    // }
}

void llvm_deref(FileCompiler *fc, LLVMValueRef v, Type *type) {
    //
    LLVMValueRef min = llvm_sub(fc, v, llvm_int(1));
    LLVMValueRef cond = llvm_icmp(fc, min, llvm_int(0));
    if (type->nullable) {
        LLVMValueRef isset = llvm_isset(fc, v);
        cond = LLVMBuildAnd(fc->builder, cond, isset, "tmp");
    }
    LLVMBasicBlockRef then = LLVMAppendBasicBlock(fc->current_func, "if_deref");
    LLVMBasicBlockRef next = LLVMAppendBasicBlock(fc->current_func, "if_deref_next");
    LLVMBuildCondBr(fc->builder, cond, then, NULL);
    //
    LLVMPositionBuilderAtEnd(fc->builder, then);
    fc->current_block = then;
    //

    //
    LLVMPositionBuilderAtEnd(fc->builder, next);
    fc->current_block = next;

    // str_append_chars(fc->tkn_buffer, "if(");
    // if (rt->nullable) {
    //     str_append_chars(fc->tkn_buffer, vb);
    //     str_append_chars(fc->tkn_buffer, " && ");
    // }
    // str_append_chars(fc->tkn_buffer, "--");
    // str_append_chars(fc->tkn_buffer, vb);
    // str_append_chars(fc->tkn_buffer, "->_RC == 0) ");
    // if (ignore_vbuf && strcmp(ignore_vbuf, vb) == 0) {
    //     str_append_chars(fc->tkn_buffer, "{}\n");
    // } else {
    //     str_append_chars(fc->tkn_buffer, rt->class->cname);
    //     str_append_chars(fc->tkn_buffer, "____free(");
    //     str_append_chars(fc->tkn_buffer, vb);
    //     str_append_chars(fc->tkn_buffer, ");\n");
    // }
}

void llvm_upref(FileCompiler *fc, LLVMValueRef v, bool nullable) {
    //
    // bool nullable = decl->value->return_type->nullable;
    // if (nullable) {
    //    str_append_chars(fc->tkn_buffer, "if(");
    //    str_append_chars(fc->tkn_buffer, decl->name);
    //    str_append_chars(fc->tkn_buffer, ") ");
    //}
    // str_append_chars(fc->tkn_buffer, decl->name);
    // str_append_chars(fc->tkn_buffer, "->_RC++;\n");
}

char *llvm_buf(FileCompiler *fc) {
    sprintf(fc->sprintf, "%d", fc->llvmc);
    fc->llvmc++;
    return fc->sprintf;
}

// Values
LLVMValueRef llvm_u8(char v) { return LLVMConstInt(LLVMInt8Type(), v, false); }
LLVMValueRef llvm_int(int v) { return LLVMConstInt(LLVMInt32Type(), v, true); }
LLVMValueRef llvm_null() { return LLVMConstInt(LLVMIntPtrType(g_target_data), 0, false); }

// Types
LLVMTypeRef llvm_ptr() { return LLVMIntPtrType(g_target_data); }

// Operands
LLVMValueRef llvm_icmp(FileCompiler *fc, LLVMValueRef left, LLVMValueRef right) { return LLVMBuildICmp(fc->builder, LLVMICmp, left, right, "icmp_1"); }
LLVMValueRef llvm_sub(FileCompiler *fc, LLVMValueRef left, LLVMValueRef right) { return LLVMBuildSub(fc->builder, left, right, "sub_1"); }

//
LLVMValueRef llvm_isset(FileCompiler *fc, LLVMValueRef v) {
    //
    return LLVMBuildIsNull(fc->builder, v, "isset_1");
}

LLVMValueRef llvm_get_allocator(FileCompiler *fc, int size, bool threaded) {
    size += 24;
    threaded = false; // force unthreaded

    if (fc->should_recompile) {
        sprintf(fc->sprintf, "%d", size);
        char *get = map_get(fc->cache->allocators, fc->sprintf);
        if (!get) {
            sprintf(fc->sprintf2, "%d", threaded);
            map_set(fc->cache->allocators, strdup(fc->sprintf), strdup(fc->sprintf2));
        }
    }

    //
    sprintf(fc->sprintf, "KI_allocator_%d_%d", size, threaded);
    char *name = fc->sprintf;

    LLVMValueRef last = map_get(allocators, name);
    if (last) {
        return last;
    }

    name = strdup(name);

    LLVMTypeRef ftype = LLVMFunctionType(llvm_ptr(), NULL, 0, 0);
    LLVMValueRef fv = LLVMAddFunction(fc->mod, name, ftype);

    map_set(allocators, name, fv);

    return fv;
}

void fc_write_c_write_allocator(Str *code, Str *hcode, char *name, char *size, bool threaded) {

    str_append_chars(hcode, threaded ? "struct pthread_key_t " : "void* ");
    str_append_chars(hcode, name);
    str_append_chars(hcode, "__TK;\n");

    str_append_chars(code, threaded ? "struct pthread_key_t " : "void* ");
    str_append_chars(code, name);
    str_append_chars(code, "__TK;\n");

    str_append_chars(hcode, "struct ki__mem__Allocator* ");
    str_append_chars(hcode, name);
    str_append_chars(hcode, "();\n");

    str_append_chars(code, "struct ki__mem__Allocator* ");
    str_append_chars(code, name);
    str_append_chars(code, "(){\n");

    str_append_chars(code, "struct ki__mem__Allocator* a = ");
    str_append_chars(code, threaded ? "pthread_getspecific(" : "(");
    str_append_chars(code, name);
    str_append_chars(code, "__TK);\n");

    str_append_chars(code, "if(a){ return a; }\n");

    str_append_chars(code, "a = ki__mem__Allocator__make(");
    str_append_chars(code, size);
    str_append_chars(code, ");\n");

    if (threaded) {
        str_append_chars(code, "pthread_setspecific(");
        str_append_chars(code, name);
        str_append_chars(code, "__TK, a);\n");
    } else {
        str_append_chars(code, name);
        str_append_chars(code, "__TK = a;\n");
    }

    str_append_chars(code, "return a;\n");
    str_append_chars(code, "}\n");
}

int llvm_prop_index(Class *class, char *prop_name) {
    //
    ClassProp *prop = map_get(class->props, prop_name);
    return prop->struct_index;
}

void llvm_build_o(LLVMModuleRef mod, char *outpath) {
    //
    char *error = NULL;
    LLVMVerifyModule(mod, LLVMAbortProcessAction, &error);

    LLVMSetTarget(mod, LLVM_DEFAULT_TARGET_TRIPLE);

    LLVMPassManagerRef pass = LLVMCreatePassManager();
    LLVMAddCFGSimplificationPass(pass);
    LLVMAddReassociatePass(pass);
    LLVMAddPromoteMemoryToRegisterPass(pass);

    LLVMPassManagerBuilderRef passBuilder = LLVMPassManagerBuilderCreate();
    LLVMPassManagerBuilderSetOptLevel(passBuilder, 3);
    LLVMPassManagerBuilderPopulateModulePassManager(passBuilder, pass);

    LLVMRunPassManager(pass, mod);

    if (LLVMTargetMachineEmitToFile(g_target_machine, mod, outpath, LLVMObjectFile, &error) != 0) {
        fprintf(stderr, "Failed to emit machine code!\n");
        fprintf(stderr, "%s\n", error);
        LLVMDisposeMessage(error);
        exit(1);
    }

    LLVMDisposeMessage(error);
    LLVMPassManagerBuilderDispose(passBuilder);
    LLVMDisposePassManager(pass);
    LLVMDisposeModule(mod);

    printf("Write: %s\n", outpath);
}
