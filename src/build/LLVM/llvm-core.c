
#include "../../headers/LLVM.h"

void llvm_build_ir(LB *b) {
    //
    Str *ir = b->ir_final;

    // llvm_gen_global_ir(b);
    // llvm_gen_func_ir(b);

    str_append_chars(ir, "; ModuleID = '");
    str_append_chars(ir, b->fc->path_ki);
    str_append_chars(ir, "'\n");

    str_append_chars(ir, "source_filename = \"");
    str_append_chars(ir, b->fc->path_ki);
    str_append_chars(ir, "\"\n");

    str_append_chars(ir, "target datalayout = \"e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128\"\n");
    str_append_chars(ir, "target triple = \"x86_64-pc-linux-gnu\"\n\n");
}
