
#include "../../headers/LLVM.h"

void llvm_ir_jump(Str *ir, LLVMBlock *block) {
    //
    str_append_chars(ir, "  br label %");
    str_append_chars(ir, block->name);
    str_append_chars(ir, "\n");
}
