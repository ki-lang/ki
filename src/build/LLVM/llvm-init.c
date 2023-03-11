
#include "../../headers/LLVM.h"

LLVMBlock *llvm_block_init(LB *b, int nr) {

    char *name = al(b->alc, 20);
    sprintf(name, "block_%d", nr);

    LLVMBlock *block = al(b->alc, sizeof(LLVMBlock));
    block->name = name;
    block->ir = str_make(b->alc, 100);
    return block;
}

LLVMFunc *llvm_func_init(LB *b, Func *func, LLVMBlock *entry, LLVMBlock *code) {
    LLVMFunc *lfunc = al(b->alc, sizeof(LLVMFunc));
    lfunc->b = b;
    lfunc->func = func;
    lfunc->ir = str_make(b->alc, 1000);

    lfunc->block = code;
    lfunc->block_entry = entry;
    lfunc->block_code = code;
    lfunc->blocks = array_make(b->alc, 20);

    lfunc->varc = 0;
    lfunc->blockc = 2;
    lfunc->stack_save_vn = NULL;

    array_push(lfunc->blocks, entry);
    array_push(lfunc->blocks, code);

    return lfunc;
}
