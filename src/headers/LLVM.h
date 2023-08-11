
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
char *llvm_get_global(LB *b, char *name, Type *type);
char *llvm_attr(LB *b);

// Func
void llvm_gen_func_ir(LB *b);
Str *llvm_func_collect_ir(LLVMFunc *lfunc);
void llvm_define_ext_func(LB *b, Func *func);

// Value
char *llvm_value(LB *b, Scope *scope, Value *v);
char *llvm_assign_value(LB *b, Scope *scope, Value *v);

// Type
void llvm_check_defined(LB *b, Class *class);
char *llvm_type(LB *b, Type *type);
char *llvm_type_real(LB *b, Type *type);
char *llvm_type_int(LB *b, int bytes);
char *llvm_type_ixx(LB *b);
char *llvm_di_type(LB *b, Type *type);

// Ast
void llvm_write_ast(LB *b, Scope *scope);

// IR
char *llvm_ir_isnull_i1(LB *b, char *ltype, char *val);
char *llvm_ir_notnull_i1(LB *b, char *ltype, char *val);
char *llvm_ir_iszero_i1(LB *b, char *ltype, char *val);
char *llvm_ir_istrue_i1(LB *b, char *val);
char *llvm_ir_cmp(LB *b, char *ltype, char *val, char *cmd, char *with);
void llvm_ir_jump(Str *ir, LLVMBlock *block);
void llvm_ir_jump_loop(LB *b, LLVMBlock *block);
void llvm_ir_cond_jump(LB *b, Str *ir, char *var_i1, LLVMBlock *a_block, LLVMBlock *b_block);
void llvm_ir_store(LB *b, Type *type, char *var, char *val);
char *llvm_ir_load(LB *b, Type *type, char *var);
char *llvm_ir_bool_i1(LB *b, Str *ir, char *val);
char *llvm_ir_class_prop_access(LB *b, Class *class, char *on, ClassProp *prop);
Array *llvm_ir_fcall_args(LB *b, Scope *scope, Array *values);
char *llvm_ir_fcall_arg(LB *b, char *lval, char *ltype);
char *llvm_ir_func_call(LB *b, char *on, Array *values, char *lrett, FCallOr * or, int line, int col);
char *llvm_ir_func_ptr(LB *b, Func *func);
char *llvm_ir_cast(LB *b, char *lval, Type *from_type, Type *to_type);
char *llvm_ir_string(LB *b, char *body);
char *llvm_ir_stack_alloc(LB *b, char *amount, char *amount_type);
char *llvm_ir_gep(LB *b, char *type, char *lon, char *index, char *index_type);

struct LB {
    Fc *fc;
    Allocator *alc;
    LLVMFunc *lfunc;
    Array *lfuncs;
    Array *defined_classes;
    Array *declared_funcs;
    Map *globals;
    Str *ir_final;
    Str *ir_struct;
    Str *ir_global;
    Str *ir_extern_func;
    Str *ir_attr;
    LLVMBlock *while_cond;
    LLVMBlock *while_after;
    char *char_buf;
    int strc;
    int attrc;
    bool use_stack_save;
    // Buffers
    Str *str_buf;
    Str *str_buf_di_type;
    // Attributes
    char *loop_attr;
    char *loop_attr_root;
    Array *attrs;
    // Debug info
    bool debug;
    char *di_cu; // Compile unit
    char *di_file;
    char *di_retained_nodes;
    // Debug info types
    char *di_type_ptr;
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
    char *stack_save_vn;
    LLVMBlock *block_entry;
    LLVMBlock *block_code;
    int varc;
    int blockc;
    // Debug info
    char *di_scope;
};