
#include "../all.h"
#include "../libs/nxjson.h"

typedef struct PkgCompiler {
    char *name;
    char *dir;
    const nx_json *config;
    char *config_path;
    struct Map *namespaces;
    struct Map *package_dirs;
    struct Map *namespace_dirs;
    struct Map *file_compilers;
    struct Map *headers;
} PkgCompiler;

typedef struct NsCompiler {
    char *name;
    struct PkgCompiler *pkc;
    struct Scope *scope;
} NsCompiler;

typedef struct FileCompiler {
    struct NsCompiler *nsc;
    //
    char *hash;
    char *ki_filepath;
    char *c_filepath;
    char *h_filepath;
    char *o_filepath;
    char *cache_filepath;
    char *x_filepath;
    bool is_header;
    bool is_used;
    bool was_modified;
    bool should_recompile;
    //
    struct FcCache *cache;
    //
    char *content;
    int content_len;
    //
    int i;
    int line;
    int col;
    // Macros
    Array *macro_results;
    Array *macro_prop_loops;
    char *macro_tag;
    // Write c
    Str *c_code;
    Str *c_code_after;
    Str *h_code_start;
    Str *h_code;
    Str *tkn_buffer;
    Str *before_tkn_buffer;
    Str *value_buffer;
    int indent;
    struct Scope *current_scope;
    struct Scope *current_func_scope;
    int var_bufc;
    char *var_buf;
    // Misc
    char *sprintf;
    char *sprintf2;
    char *add_use_target;
    // Local identifiers
    struct Scope *scope;
    // Things to compile in this file
    bool create_o_file;
    struct Array *functions;
    struct Array *classes;
    struct Array *enums;
    struct Array *strings;
    struct Array *globals;
    struct Array *used_functions;
    struct Array *converter_positions;
    // Extern
    struct Array *include_headers_from;
    // LLVM
    LLVMModuleRef mod;
    LLVMBuilderRef builder;
} FileCompiler;

typedef struct FcCache {
    int modified_time;
    int tests_enabled;
    struct Map *depends_on;
    struct Map *allocators;
} FcCache;

typedef struct Scope {
    int type;
    struct Map *identifiers;
    bool is_func;
    bool is_vscope;
    bool is_loop;
    bool in_loop;
    bool must_return;
    bool did_return;
    bool catch_errors;
    struct Class *class;
    struct Function *func;
    int body_i;
    int body_i_end;
    struct Array *ast;
    struct Scope *parent;
    // Value scope
    char *vscope_vname;
    struct Type *vscope_return_type;
    //
    Array *var_bufs;
    Array *local_var_names;
} Scope;

typedef enum ScopeType {
    sct_unknown,
    sct_class,
} ScopeType;

typedef struct Identifier {
    char *package;
    char *namespace;
    char *name;
} Identifier;

typedef struct IdentifierFor {
    int type;
    void *item;
} IdentifierFor;

typedef enum IdentifierForType {
    idfor_unknown,
    idfor_func,
    idfor_class,
    idfor_enum,
    idfor_local_var,
    idfor_type, // 5
    idfor_property,
    idfor_trait,
    idfor_arg,
    idfor_threaded_global,
    idfor_shared_global,
    idfor_namespace,
    idfor_macro_token,
    idfor_converter,
} IdentifierForType;

//////////

typedef struct ContentChunk {
    FileCompiler *fc;
    int i;
} ContentChunk;

typedef struct Class {
    char *cname;
    char *name;
    char *hash;
    struct FileCompiler *fc;
    //
    struct Scope *scope;
    bool ref_count;
    bool is_number;
    bool is_float;
    bool is_unsigned;
    bool is_ctype;
    bool self_scan;
    int size;
    //
    Map *props;
    //
    int body_i;
    int body_i_end;
    //
    Array *traits;
    Array *generic_names;
    char *generic_hash;
} Class;

typedef struct ClassProp {
    int access_type;
    bool is_static;
    bool is_func;
    bool generate_code;
    struct Type *return_type;
    struct Value *default_value;
    int value_i;
    struct Function *func;
    char *macro_tag;
} ClassProp;

typedef enum ClassPropAccType {
    acct_unknown,
    acct_public,
    acct_readonly,
    acct_private,
} ClassPropAccType;

typedef struct Function {
    char *cname;
    char *hash;
    struct FileCompiler *fc;
    bool can_error;
    bool generate_code;
    bool is_test;
    bool accesses_globals;
    //
    struct Array *args;
    struct Array *arg_types;
    struct Type *return_type;
    //
    int args_i;
    int args_i_end;
    //
    Scope *scope;
    //
    struct Array *called_by;
} Function;

typedef struct FunctionArg {
    char *name;
    struct Type *type;
    struct Value *default_value;
} FunctionArg;

typedef struct Converter {
    char *cname;
    Array *from_types;
    Array *to_types;
    Array *functions;
} Converter;

typedef struct ConverterPos {
    int fc_i;
    Converter *converter;
} ConverterPos;

typedef struct GlobalVar {
    FileCompiler *fc;
    int fc_i;
    int type;
    struct Type *return_type;
    char *name;
    char *cname;
    int or_type;
    Scope *vscope;
    char *error_msg;
    struct Value *default_value;
} GlobalVar;

typedef enum GlobalVarType {
    gv_shared,
    gv_threaded,
} GlobalVarType;

typedef struct Trait {
    char *cname;
    struct FileCompiler *fc;
    int body_i;
} Trait;

typedef struct Enum {
    char *name;
    char *cname;
    char *hash;
    struct Map *values;
    FileCompiler *fc;
} Enum;

//////////

typedef struct Type {
    int type;
    bool nullable;
    bool npt;
    bool allow_math;
    bool is_pointer;
    bool is_pointer_of_pointer;
    bool is_array;
    bool is_float;
    bool is_unsigned;
    bool c_static;
    bool c_inline;
    //
    int array_size;
    short bytes;
    Class *class;
    Enum *enu;
    // Func ref
    Array *func_arg_types;
    struct Type *func_return_type;
    bool func_can_error;
} Type;

typedef enum TypeType {
    type_unknown,
    type_void,
    type_void_pointer,
    type_null,
    type_funcref,
    type_struct,
    type_union,
    type_bool,
    type_enum,
    type_number,
    type_throw_msg,
} TypeType;

typedef struct Value {
    int type;
    Type *return_type;
    void *item;
} Value;
typedef enum ValueType {
    vt_unknown,
    vt_null,
    vt_true,
    vt_false,
    vt_group,
    vt_number, // 5
    vt_string,
    vt_char,
    vt_operator,
    vt_var,
    vt_func_call, // 10
    vt_sizeof,
    vt_cast,
    vt_getptr,
    vt_getptrv,
    vt_setptrv, // 15
    vt_class_init,
    vt_prop_access,
    vt_async,
    vt_await,
    vt_allocator, // 20
    vt_arg,
    vt_threaded_global,
    vt_shared_global,
    vt_nullable_value,
    vt_null_or,
} ValueType;

typedef struct ValueFuncCall {
    Array *arg_values;
    Value *on;
    struct OrToken *ort;
} ValueFuncCall;

typedef struct ErrorToken {
    int type;
    char *msg;
    char *filepath;
    int line;
    int col;
} ErrorToken;

typedef enum ErrorType {
    err_throw,
    err_exit,
    err_panic,
} ErrorType;

typedef struct OrToken {
    int type;
    Scope *vscope;
    Value *value;
    Scope *else_scope;
    ErrorToken *error;
    Type *primary_type;
    char *error_vn;
} OrToken;

typedef enum OrType {
    or_none,
    or_pass,
    or_value,
    or_return,
    or_throw,
    or_do,
    or_set,
    or_exit,
    or_panic,
    or_break,
    or_continue,
} OrType;

typedef struct ValueOperator {
    int type;
    Value *left;
    Value *right;
} ValueOperator;

typedef enum OperatorType {
    op_sub,
    op_add,
    op_mult,
    op_div,
    op_mod,
    op_eq, // 5
    op_neq,
    op_lte,
    op_gte,
    op_lt,
    op_gt, // 10
    op_incr,
    op_decr,
    op_bit_OR,
    op_bit_AND,
    op_bit_XOR, // 15
    op_and,
    op_or,
    op_bit_shift_left,
    op_bit_shift_right,
    op_null_or,
} OperatorType;

typedef struct ValueCast {
    Value *value;
    Type *as_type;
} ValueCast;

typedef struct SetPtrValue {
    Value *ptr_value;
    Value *to_value;
} SetPtrValue;

typedef struct ValueClassInit {
    Class *class;
    Map *prop_values;
} ValueClassInit;

typedef struct ValueClassPropAccess {
    void *on;
    char *name;
    bool is_static;
} ValueClassPropAccess;

typedef struct ValueString {
    char *name;
    char *body;
} ValueString;

//////////

typedef struct Token {
    int type;
    void *item;
} Token;

typedef enum TokenTypeEnum {
    tkn_unknown,
    tkn_return,
    tkn_set_vscope_value,
    tkn_while,
    tkn_if,
    tkn_ifnull, // 5
    tkn_notnull,
    tkn_throw,
    tkn_break,
    tkn_continue,
    tkn_free, // 10
    tkn_value,
    tkn_declare,
    tkn_assign,
    tkn_static,
    tkn_set_threaded,
    tkn_mutex_init,
    tkn_mutex_lock,
    tkn_mutex_unlock,
    tkn_exit,
    tkn_panic,
    tkn_each,
    // Global ast only
    tkn_func,
    tkn_class,
    // misc
    tkn_debug_msg,
    tkn_init_thread,
    //
} TokenTypeEnum;

typedef struct TokenSetVscopeValue {
    char *vname;
    Value *value;
    Scope *vscope;
} TokenSetVscopeValue;

typedef struct TokenEach {
    Value *value;
    struct LocalVar *kvar;
    struct LocalVar *vvar;
    Scope *scope;
} TokenEach;

typedef struct TokenIf {
    Value *condition;
    bool is_else;
    struct TokenIf *next;
    struct Scope *scope;
} TokenIf;

typedef struct TokenIfNull {
    int type;
    char *name;
    IdentifierFor *idf;
    struct OrToken *ort;
} TokenIfNull;

typedef struct TokenNotNull {
    int type;
    char *name;
    struct Scope *scope;
    struct Scope *else_scope;
} TokenNotNull;

typedef struct TokenWhile {
    Value *condition;
    struct Scope *scope;
} TokenWhile;

typedef struct TokenDeclare {
    char *name;
    struct Value *value;
    struct Type *type;
} TokenDeclare;
typedef struct TokenAssign {
    int type;
    struct Value *left;
    struct Value *right;
} TokenAssign;

typedef struct TokenThrow {
    char *msg;
    Type *return_type;
} TokenThrow;

typedef struct LocalVar {
    char *name;
    char *gen_name;
    Type *type;
} LocalVar;

typedef struct VarInfo {
    char *name;
    Type *return_type;
} VarInfo;

typedef struct TokenIdValue {
    char *name;
    Value *value;
} TokenIdValue;

typedef struct PropLoop {
    int fc_i;
    Class *class;
    int prop_index;
    char *name_id;
    char *type_id;
    char *filter;
} PropLoop;
