
#include "../../headers/LLVM.h"

Str *llvm_b_ir(LB *b) {
    //
    return b->lfunc->block->ir;
}

void llvm_build_ir(LB *b) {
    //
    Str *ir = b->ir_final;
    Build *bld = b->fc->b;

    llvm_gen_global_ir(b);
    llvm_gen_func_ir(b);

    str_append_chars(ir, "; ModuleID = '");
    str_append_chars(ir, b->fc->path_ki);
    str_append_chars(ir, "'\n");

    str_append_chars(ir, "source_filename = \"");
    str_append_chars(ir, b->fc->path_ki);
    str_append_chars(ir, "\"\n");

    // str_append_chars(ir, "target datalayout = \"e-m:o-i64:64-i128:128-n32:64-S128\"\n");
    str_append_chars(ir, "target datalayout = \"e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128\"\n");

    if (strcmp(bld->os, "linux") == 0) {
        if (strcmp(bld->arch, "x64") == 0) {
            str_append_chars(ir, "target triple = \"x86_64-pc-linux-gnu\"");
        } else if (strcmp(bld->arch, "arm64") == 0) {
            str_append_chars(ir, "target triple = \"aarch64-unknown-linux-gnu\"");
        }
    } else if (strcmp(bld->os, "macos") == 0) {
        if (strcmp(bld->arch, "x64") == 0) {
            str_append_chars(ir, "target triple = \"x86_64-apple-darwin\"");
        } else if (strcmp(bld->arch, "arm64") == 0) {
            str_append_chars(ir, "target triple = \"aarch64-apple-darwin\"");
        }
    } else if (strcmp(bld->os, "win") == 0) {
        if (strcmp(bld->arch, "x64") == 0) {
            str_append_chars(ir, "target triple = \"x86_64-pc-windows-msvc\"");
        } else if (strcmp(bld->arch, "arm64") == 0) {
            str_append_chars(ir, "target triple = \"aarch64-pc-windows-msvc\"");
        }
    }

    str_append_chars(ir, "\n\n");

    str_append(ir, b->ir_struct);
    str_append_chars(ir, "\n");
    str_append(ir, b->ir_global);
    str_append_chars(ir, "\n");

    for (int i = 0; i < b->lfuncs->length; i++) {
        LLVMFunc *lfunc = array_get_index(b->lfuncs, i);
        str_append(ir, lfunc->ir);
    }
    str_append_chars(ir, "\n");

    str_append(ir, b->ir_extern_func);
    str_append_chars(ir, "\n");

    if (b->use_stack_save) {
        str_append_chars(ir, "declare i8* @llvm.stacksave()\n");
        str_append_chars(ir, "declare void @llvm.stackrestore(i8*)\n\n");
    }

    str_append_chars(ir, "!0 = !{!\"llvm.loop.mustprogress\"}\n");
    str_append(ir, b->ir_attr);
    str_append_chars(ir, "\n");
}

void llvm_gen_global_ir(LB *b) {
    //
    Fc *fc = b->fc;
    Scope *scope = fc->scope;
    Str *ir = b->ir_global;

    bool is_main_fc = fc->b->main_func->fc == fc;

    char bytes[20];
    sprintf(bytes, "%d", fc->b->ptr_size);

    if (is_main_fc) {
        str_append_chars(ir, "@ki_err_code_buffer = dso_local thread_local(initialexec) global i32 0, align 4\n");
        str_append_chars(ir, "@ki_err_msg_buffer = dso_local thread_local(initialexec) global i8* null, align ");
        str_append_chars(ir, bytes);
        str_append_chars(ir, "\n");
    } else {
        str_append_chars(ir, "@ki_err_code_buffer = external thread_local(initialexec) global i32, align 4\n");
        str_append_chars(ir, "@ki_err_msg_buffer = external thread_local(initialexec) global i8*, align ");
        str_append_chars(ir, bytes);
        str_append_chars(ir, "\n");
    }

    for (int i = 0; i < fc->globals->length; i++) {
        Global *g = array_get_index(fc->globals, i);

        char *name = g->gname;
        Type *type = g->type;

        char *ltype = llvm_type(b, type);
        str_append_chars(ir, "@");
        str_append_chars(ir, name);
        str_append_chars(ir, " = dso_local ");
        str_append_chars(ir, g->shared ? "" : "thread_local(initialexec) ");
        str_append_chars(ir, " global ");
        str_append_chars(ir, ltype);
        str_append_chars(ir, " ");
        if (type->ptr_depth > 0) {
            str_append_chars(ir, "null");
        } else {
            str_append_chars(ir, "0");
        }

        char bytes[20];
        int abytes = type->bytes;
        if (abytes > b->fc->b->ptr_size) {
            abytes = b->fc->b->ptr_size;
        }
        sprintf(bytes, "%d", abytes);

        str_append_chars(ir, ", align ");
        str_append_chars(ir, bytes);
        str_append_chars(ir, "\n");

        char *val = al(b->alc, strlen(name) + 2);
        strcpy(val, "@");
        strcat(val, name);

        map_set(b->globals, name, val);
    }
}

char *llvm_var(LB *b) {
    LLVMFunc *lfunc = b->lfunc;
    char *res = al(b->alc, 10);
    char nr[20];
    sprintf(nr, "%d", lfunc->varc);
    strcpy(res, "%.");
    strcat(res, nr);
    lfunc->varc++;
    return res;
}

char *llvm_alloca(LB *b, Type *type) {
    //
    LLVMFunc *lfunc = b->lfunc;
    LLVMBlock *block = lfunc->block_entry;
    Str *ir = block->ir;

    char bytes[20];
    int abytes = type->bytes;
    if (abytes > b->fc->b->ptr_size) {
        abytes = b->fc->b->ptr_size;
    }
    sprintf(bytes, "%d", abytes);

    char *var = llvm_var(b);
    str_append_chars(ir, "  ");
    str_append_chars(ir, var);
    str_append_chars(ir, " = alloca ");
    str_append_chars(ir, llvm_type(b, type));
    str_append_chars(ir, ", align ");
    str_append_chars(ir, bytes);
    str_append_chars(ir, "\n");
    return var;
}

char *llvm_get_global(LB *b, char *name, Type *type) {
    //
    Scope *scope = b->fc->scope;

    char *val = map_get(b->globals, name);
    if (val) {
        return val;
    }

    Str *ir = b->ir_global;
    char *ltype = llvm_type(b, type);

    char bytes[20];
    int abytes = type->bytes;
    if (abytes > b->fc->b->ptr_size) {
        abytes = b->fc->b->ptr_size;
    }
    sprintf(bytes, "%d", abytes);

    str_append_chars(ir, "@");
    str_append_chars(ir, name);
    str_append_chars(ir, " = external global ");
    str_append_chars(ir, ltype);
    str_append_chars(ir, ", align ");
    str_append_chars(ir, bytes);
    str_append_chars(ir, "\n");

    char tmp[strlen(name) + 2];
    strcpy(tmp, "@");
    strcat(tmp, name);

    char *result = dups(b->alc, tmp);

    map_set(b->globals, name, result);

    return result;
}

char *llvm_attr(LB *b) {
    char *res = al(b->alc, 8);
    char nr[20];
    sprintf(res, "!%d", b->attrc);
    b->attrc++;
    return res;
}
