
#include "../../headers/LLVM.h"

void llvm_gen_func_ir(LB *b) {
    //
    Fc *fc = b->fc;
    Allocator *alc = b->alc;

    for (int i = 0; i < fc->funcs->length; i++) {
        Func *func = array_get_index(fc->funcs, i);
        Array *args = func->args;
        int argc = args->length;

        Scope *fscope = func->scope;

        LLVMBlock *block_entry = llvm_block_init(b, 0);
        LLVMBlock *block_code = llvm_block_init(b, 1);

        LLVMFunc *lfunc = llvm_func_init(b, func, block_entry, block_code);

        b->lfunc = lfunc;
        array_push(b->lfuncs, lfunc);

        Str *ir = lfunc->ir;

        str_append_chars(ir, "define dso_local ");
        str_append_chars(ir, llvm_type(b, func->rett));
        str_append_chars(ir, " @");
        str_append_chars(ir, func->gname);
        str_append_chars(ir, "(");

        for (int o = 0; o < argc; o++) {
            Arg *arg = array_get_index(args, o);
            if (o > 0) {
                str_append_chars(ir, ", ");
            }
            str_append_chars(ir, llvm_type(b, arg->type));
            str_append_chars(ir, " noundef ");
            if (arg->type->ptr_depth > 0 && !arg->type->nullable)
                str_append_chars(ir, " nonnull ");
            // if (arg->type->borrow)
            //     str_append_chars(ir, " nofree readnone ");
            // if (arg->type->ptr_depth > 0)
            //     str_append_chars(ir, " nocapture ");
            // if (!arg->is_mut)
            //     str_append_chars(ir, " inreg ");

            char *v = llvm_var(b);
            str_append_chars(ir, v);

            printf("func: %s | arg:%s | %p\n", func->dname, arg->name, arg->decl);
            arg->decl->llvm_val = v;
        }
        str_append_chars(ir, ")");

        if (func->opt_hot) {
            str_append_chars(ir, " hot");
        }
        if (func->opt_inline) {
            str_append_chars(ir, " alwaysinline");
        }

        // Debug info
        if (b->debug) {
            char *di_scope = llvm_attr(b);
            lfunc->di_scope = di_scope;
            str_append_chars(ir, " !dbg ");
            str_append_chars(ir, di_scope);

            char *di_func_type = llvm_attr(b);
            char *di_args = llvm_attr(b);

            char line[10];
            sprintf(line, "%d", func->line);

            Str *buf = b->str_buf;
            str_clear(buf);
            str_append_chars(buf, di_scope);
            str_append_chars(buf, " = distinct !DISubprogram(name: \"");
            str_append_chars(buf, func->gname);
            str_append_chars(buf, "\", scope: ");
            str_append_chars(buf, b->di_file);
            str_append_chars(buf, ", file: ");
            str_append_chars(buf, b->di_file);
            str_append_chars(buf, ", line: ");
            str_append_chars(buf, line);
            str_append_chars(buf, ", type: ");
            str_append_chars(buf, di_func_type);
            str_append_chars(buf, ", scopeLine: ");
            str_append_chars(buf, line);
            str_append_chars(buf, ", spFlags: DISPFlagDefinition, unit: ");
            str_append_chars(buf, b->di_cu);
            str_append_chars(buf, ", retainedNodes: ");
            str_append_chars(buf, b->di_retained_nodes);
            str_append_chars(buf, ")");

            array_push(b->attrs, str_to_chars(alc, buf));

            str_clear(buf);
            str_append_chars(buf, di_func_type);
            str_append_chars(buf, " = !DISubroutineType(types: ");
            str_append_chars(buf, di_args);
            str_append_chars(buf, ")");

            array_push(b->attrs, str_to_chars(alc, buf));

            str_clear(buf);
            str_append_chars(buf, di_args);
            str_append_chars(buf, " = !{");

            str_append_chars(buf, llvm_di_type(b, func->rett));

            for (int i = 0; i < argc; i++) {
                Arg *arg = array_get_index(args, i);
                str_append_chars(buf, ", ");
                str_append_chars(buf, llvm_di_type(b, arg->type));
            }

            str_append_chars(buf, " }");
            array_push(b->attrs, str_to_chars(alc, buf));
        }

        // Body
        str_append_chars(ir, " {\n");

        // Store mutable args in variables
        Str *cir = llvm_b_ir(b);
        for (int o = 0; o < func->args->length; o++) {

            Arg *arg = array_get_index(func->args, o);

            if (!arg->decl->is_mut) {
                continue;
            }

            char *lval = arg->decl->llvm_val;

            Type *type = arg->type;
            char *ltype = llvm_type(b, type);
            char *var = llvm_alloca(b, type);

            arg->decl->llvm_val = var;

            char bytes[20];
            int abytes = type->bytes;
            if (abytes > b->fc->b->ptr_size) {
                abytes = b->fc->b->ptr_size;
            }
            sprintf(bytes, "%d", abytes);

            str_append_chars(cir, "  store ");
            str_append_chars(cir, ltype);
            str_append_chars(cir, " ");
            str_append_chars(cir, lval);
            str_append_chars(cir, ", ptr ");
            str_append_chars(cir, var);
            str_append_chars(cir, ", align ");
            str_append_chars(cir, bytes);
            str_append_chars(cir, "\n");
        }

        if (func->uses_stack_alloc) {
            b->use_stack_save = true;
            char *save_vn = llvm_var(b);
            str_append_chars(cir, "  ");
            str_append_chars(cir, save_vn);
            str_append_chars(cir, " = call i8* @llvm.stacksave()\n");
            lfunc->stack_save_vn = save_vn;
        }

        // AST
        llvm_write_ast(b, fscope);
        if (!fscope->did_return) {
            str_append_chars(llvm_b_ir(b), "  ret void\n");
        }

        str_append(ir, llvm_func_collect_ir(lfunc));

        str_append_chars(ir, "}\n\n");

        b->lfunc = NULL;
    }
}

Str *llvm_func_collect_ir(LLVMFunc *lfunc) {
    //
    LB *b = lfunc->b;
    Allocator *alc = b->alc;

    int size = 0;
    for (int i = 0; i < lfunc->blocks->length; i++) {
        LLVMBlock *block = array_get_index(lfunc->blocks, i);
        size += block->ir->length;
    }

    Str *result = str_make(alc, size);
    for (int i = 0; i < lfunc->blocks->length; i++) {
        LLVMBlock *block = array_get_index(lfunc->blocks, i);
        if (i == 0 && block->ir->length == 0) {
            continue;
        }
        str_append_chars(result, block->name);
        str_append_chars(result, ":\n");
        str_append(result, block->ir);
        if (block == lfunc->block_entry) {
            LLVMBlock *b_code = lfunc->block_code;
            if (b_code->ir->length > 0) {
                llvm_ir_jump(result, b_code);
            }
        }
        str_append_chars(result, "\n");
    }
    return result;
}

void llvm_define_ext_func(LB *b, Func *func) {

    Array *decls = b->declared_funcs;

    if (array_contains(decls, func, arr_find_adr))
        return;

    array_push(decls, func);

    Str *ir = b->ir_extern_func;

    str_append_chars(ir, "declare ");
    str_append_chars(ir, llvm_type(b, func->rett));
    str_append_chars(ir, " @");
    str_append_chars(ir, func->gname);
    str_append_chars(ir, "(");
    int argc = func->args->length;
    for (int o = 0; o < func->args->length; o++) {
        Arg *arg = array_get_index(func->args, o);
        if (o > 0) {
            str_append_chars(ir, ", ");
        }
        str_append_chars(ir, llvm_type(b, arg->type));
        str_append_chars(ir, " noundef");
        if (arg->type->ptr_depth > 0 && !arg->type->nullable)
            str_append_chars(ir, " nonnull");
    }
    // if (func->can_error) {
    //     if (argc > 0) {
    //         str_append_chars(ir, ", ");
    //     }
    //     str_append_chars(ir, "i32* noundef, i8** noundef");
    // }
    str_append_chars(ir, ")\n");
}