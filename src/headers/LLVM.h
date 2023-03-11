
#include "../all.h"

typedef struct LB LB;
typedef struct LLVMBlock LLVMBlock;
typedef struct LLVMFunc LLVMFunc;

void llvm_build_ir(LB *b);

struct LB {
    Fc *fc;
    Allocator *alc;
    LLVMFunc *_lfunc;
    Array *lfuncs;
    Array *defined_classes;
    Array *defined_funcs;
    Str *ir_final;
    Str *ir_struct;
    Str *ir_global;
    Str *ir_extern_func;
    int strc;
    LLVMBlock *while_cond;
    LLVMBlock *while_after;
};

struct LLVMBlock {
    Str *ir;
};
struct LLVMFunc {
    LB *b;
    Func *func;
    LLVMBlock *block;
    Str *ir;
    Array *blocks;
    int varc;
    int blockc;
    char *stack_save_vn;
    LLVMBlock *block_entry;
    LLVMBlock *block_code;
};