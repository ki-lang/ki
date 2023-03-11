
#include "../../headers/LLVM.h"

void llvm_gen_func_ir(LB *b) {
    //
    Fc *fc = b->fc;
    Allocator *alc = b->alc;

    for (int i = 0; i < fc->funcs->length; i++) {
        Func *func = array_get_index(fc->funcs, i);

        Scope *fscope = func->scope;
        if (fscope->ast->length == 0) {
            break;
        }

        LLVMBlock *block_entry = llvm_block_init(b);
        LLVMBlock *block_code = llvm_block_init(b);

        LLVMFunc *lfunc = llvm_func_init(b, func, block_entry, block_code);

        b->lfunc = lfunc;
        array_push(b->lfuncs, lfunc);

        Str *ir = lfunc->ir;

        str_append_chars(ir, "define dso_local ");
        str_append_chars(ir, llvm_type(b, func->rett));
        str_append_chars(ir, " @");
        str_append_chars(ir, func->gname);
        str_append_chars(ir, "(");

        for (int o = 0; o < func->args->length; o++) {
            Arg *arg = array_get_index(func->args, o);
            if (o > 0) {
                str_append_chars(ir, ", ");
            }
            str_append_chars(ir, llvm_type(b, arg->type));
            str_append_chars(ir, " noundef ");

            char *v = llvm_var(b);
            str_append_chars(ir, v);

            // if func.can_error && i == (argc - 2) {
            //	b.func_arg_err = v;
            // }
            // if func.can_error && i == (argc - 1) {
            //	b.func_arg_msg = v;
            // }

            map_set(fscope->lvars, arg->name, v);
        }
        // if (func->can_error) {
        //     if argc
        //         > 0 { ir.append(", "); }
        //     let var_err = b.gen_var();
        //     let var_msg = b.gen_var();
        //     ir.append("i32* noundef ");
        //     ir.append(var_err);
        //     ir.append(", i8** noundef ");
        //     ir.append(var_msg);

        //     b.func_arg_err = var_err;
        //     b.func_arg_msg = var_msg;
        // }
        str_append_chars(ir, ") {\n");

        // Error buffers
        // if (func->scope->create_error_buffers) {
        //     b.func_buf_err = b.alloca(type_gen(b.fc.b, "i32"));
        //     b.func_buf_msg = b.alloca(type_gen(b.fc.b, "String"));
        // }

        // Store mutable args in variables
        Str *cir = llvm_b_ir(b);
        for (int o = 0; o < func->args->length; o++) {
            Arg *arg = array_get_index(func->args, o);
            if (!arg->is_mut) {
                continue;
            }
            Type *type = arg->type;
            char *ltype = llvm_type(b, type);
            char *var = llvm_alloca(b, type);

            char *lval = map_get(fscope->lvars, arg->name);

            char bytes[20];
            itoa(type->bytes, bytes, 10);

            str_append_chars(cir, "  store ");
            str_append_chars(cir, ltype);
            str_append_chars(cir, " ");
            str_append_chars(cir, lval);
            str_append_chars(cir, ", ");
            str_append_chars(cir, ltype);
            str_append_chars(cir, "* ");
            str_append_chars(cir, var);
            str_append_chars(cir, ", align ");
            str_append_chars(cir, bytes);
            str_append_chars(cir, "\n");

            map_set(fscope->lvars, arg->name, var);
        }

        // AST
        llvm_write_ast(b, fscope);
        if (!fscope->did_return) {
            str_append_chars(llvm_b_ir(b), "  ret void\n");
        }

        str_append(ir, llvm_func_collect_ir(lfunc));

        str_append_chars(ir, "}\n");

        b->lfunc = NULL;
    }
}

Str *llvm_func_collect_ir(LLVMFunc *lfunc) {
    //
    die("TODO collect lfunc IR");
}

void llvm_define_ext_func(LB *b, Func *func) {

    Array *decls = b->declared_funcs;

    if (array_contains(decls, func, "address"))
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
    }
    if (func->can_error) {
        if (argc > 0) {
            str_append_chars(ir, ", ");
        }
        str_append_chars(ir, "i32* noundef, i8** noundef");
    }
    str_append_chars(ir, ")\n");
}