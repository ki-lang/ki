
#include "../all.h"

typedef struct LB LB;
typedef struct LLVMBlock LLVMBlock;
typedef struct LLVMFunc LLVMFunc;

Str *llvm_b_ir(LB *b);
void llvm_build_ir(LB *b);
LLVMBlock *llvm_block_init(LB *b, int nr);
LLVMBlock *llvm_block_init_auto(LB *b);
LLVMFunc *llvm_func_init(LB *b, Func *func, LLVMBlock *entry, LLVMBlock *code);
void llvm_gen_global_ir(LB *b);
char *llvm_var(LB *b);
char *llvm_alloca(LB *b, Type *type);
char *llvm_get_var(LB *b, Scope *start_scope, Var *var);
char *llvm_get_global(LB *b, Var *var);

// Func
void llvm_gen_func_ir(LB *b);
Str *llvm_func_collect_ir(LLVMFunc *lfunc);
void llvm_define_ext_func(LB *b, Func *func);

// Value
char *llvm_value(LB *b, Scope *scope, Value *v);
char *llvm_assign_value(LB *b, Scope *scope, Value *v);

// Type
char *llvm_type(LB *b, Type *type);
char *llvm_type_int(LB *b, int bytes);
char *llvm_type_ixx(LB *b);

// Ast
char *llvm_write_ast(LB *b, Scope *scope);

// IR
void llvm_ir_jump(Str *ir, LLVMBlock *block);
void llvm_ir_cond_jump(LB *b, Str *ir, char *var_i1, LLVMBlock *a_block, LLVMBlock *b_block);
void llvm_ir_store(LB *b, Type *type, char *var, char *val);
char *llvm_ir_load(LB *b, Type *type, char *var);
char *llvm_ir_bool_i1(LB *b, Str *ir, char *val);
char *llvm_ir_class_prop_access(LB *b, Class *class, char *on, ClassProp *prop);
Array *llvm_ir_fcall_args(LB *b, Scope *scope, Array *values);
char *llvm_ir_func_call(LB *b, char *on, Array *values, char *lrett, bool can_error);

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
    char *name;
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