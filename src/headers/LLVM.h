
#include "../all.h"

typedef struct LB LB;
typedef struct LLVMBlock LLVMBlock;
typedef struct LLVMFunc LLVMFunc;

Str *llvm_b_ir(LB *b);
void llvm_build_ir(LB *b);
LLVMBlock *llvm_block_init(LB *b);
LLVMBlock *llvm_func_init(LB *b, Func *func, LLVMBlock *entry, LLVMBlock *code);
void llvm_gen_global_ir(LB *b);

// Func
void llvm_gen_func_ir(LB *b);
Str *llvm_func_collect_ir(LLVMFunc *lfunc);
void llvm_define_ext_func(LB *b, Func *func);

struct LB {
    Fc *fc;
    Allocator *alc;
    LLVMFunc *lfunc;
    Array *lfuncs;
    Array *defined_classes;
    Array *declared_funcs;
    Str *ir_final;
    Str *ir_struct;
    Str *ir_global;
    Str *ir_extern_func;
    int strc;
    LLVMBlock *while_cond;
    LLVMBlock *while_after;
    bool use_stack_save;
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