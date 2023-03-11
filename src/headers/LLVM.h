
#include "../all.h"

typedef struct LB LB;
typedef struct LLVMBlock LLVMBlock;
typedef struct LLVMFunc LLVMFunc;

Str *llvm_b_ir(LB *b);
void llvm_build_ir(LB *b);
LLVMBlock *llvm_block_init(LB *b);
LLVMFunc *llvm_func_init(LB *b, Func *func, LLVMBlock *entry, LLVMBlock *code);
void llvm_gen_global_ir(LB *b);
char *llvm_var(LB *b);
char *llvm_alloca(LB *b, Type *type);

// Func
void llvm_gen_func_ir(LB *b);
Str *llvm_func_collect_ir(LLVMFunc *lfunc);
void llvm_define_ext_func(LB *b, Func *func);

// Value
char *llvm_value(LB *b, Value *v);

// Type
char *llvm_type(LB *b, Type *type);
char *llvm_type_int(LB *b, int bytes);
char *llvm_type_ixx(LB *b);

// Ast
char *llvm_write_ast(LB *b, Scope *scope);

// IR

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
    Str *str_buf;
    char *char_buf;
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