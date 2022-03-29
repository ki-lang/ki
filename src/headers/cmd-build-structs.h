
#include "../all.h"
#include "../libs/nxjson.h"

typedef struct PkgCompiler {
  char* name;
  char* dir;
  const nx_json* config;
  char* config_path;
  struct Map* namespaces;
  struct Map* package_dirs;
  struct Map* namespace_dirs;
  struct Map* file_compilers;
  struct Map* headers;
} PkgCompiler;

typedef struct NsCompiler {
  char* name;
  struct PkgCompiler* pkc;
  struct Scope* scope;
} NsCompiler;

typedef struct FileCompiler {
  struct NsCompiler* nsc;
  //
  char* hash;
  char* ki_filepath;
  char* c_filepath;
  char* h_filepath;
  char* o_filepath;
  bool is_header;
  //
  char* content;
  int content_len;
  //
  int i;
  int line;
  int col;
  // Macros
  Array* macro_results;
  // Write c
  Str* c_code;
  Str* c_code_after;
  Str* struct_code;  // Structs
  Str* h_code;       // Function/Vars
  Str* tkn_buffer;
  Str* before_tkn_buffer;
  Str* value_buffer;
  int indent;
  int var_bufc;
  char* var_buf;
  Array* var_bufs;
  Array* local_var_names;
  // Misc
  char* sprintf;
  // Local identifiers
  struct Scope* scope;
  struct Map* uses;
  struct Map* c_imports;
  // Things to compile in this file
  bool create_o_file;
  struct Array* functions;
  struct Array* classes;
  struct Array* enums;
  struct Array* threaded_globals;
  struct Array* mutexes;
  struct Array* static_vars;
  // Extern
  struct Array* include_headers_from;
} FileCompiler;

typedef struct Scope {
  struct Map* identifiers;
  bool is_func;
  bool in_loop;
  bool must_return;
  bool did_return;
  bool catch_errors;
  bool autofill_return_type;
  int body_i;
  int body_i_end;
  struct Array* ast;
  struct Scope* parent;
  struct Type* return_type;
} Scope;

typedef struct Identifier {
  char* package;
  char* namespace;
  char* name;
  char* generic_hash;
} Identifier;

typedef struct IdentifierFor {
  int type;
  void* item;
} IdentifierFor;

typedef enum IdentifierForType {
  idfor_unknown,
  idfor_func,
  idfor_class,
  idfor_enum,
  idfor_var,
  idfor_type,  // 5
  idfor_property,
  idfor_trait,
  idfor_threaded_var,
  idfor_static_var,
  idfor_mutex,
} IdentifierForType;

//////////

typedef struct ContentChunk {
  FileCompiler* fc;
  int i;
} ContentChunk;

typedef struct FcUse {
  PkgCompiler* pkc;
  NsCompiler* nsc;
} FcUse;

typedef struct Class {
  char* cname;
  char* name;
  struct FileCompiler* fc;
  bool ref_count;
  bool is_number;
  bool is_float;
  bool is_unsigned;
  bool is_ctype;
  int size;
  //
  Map* props;
  //
  int body_i;
  int body_i_end;
  //
  Array* traits;
  Array* generic_names;
  Map* generic_types;
} Class;

typedef struct ClassProp {
  int access_type;
  bool is_static;
  bool is_func;
  struct Type* return_type;
  struct Value* default_value;
  int value_i;
  struct Function* func;
} ClassProp;

typedef enum ClassPropAccType {
  acct_public,
  acct_readonly,
  acct_private,
} ClassPropAccType;

typedef struct Function {
  char* cname;
  struct FileCompiler* fc;
  bool can_error;
  //
  struct Array* args;
  struct Type* return_type;
  //
  int args_i;
  int args_i_end;
  //
  Scope* scope;
} Function;

typedef struct FunctionArg {
  char* name;
  struct Type* type;
  struct Value* default_value;
} FunctionArg;

typedef struct Trait {
  char* cname;
  struct FileCompiler* fc;
  int body_i;
} Trait;

typedef struct Enum {
  char* cname;
  char* name;
  struct Map* values;
} Enum;

typedef struct ThreadedGlobal {
  int i;
  struct Type* type;
  char* name;
  struct Value* default_value;
} ThreadedGlobal;

typedef struct Mutex {
  char* name;
  char* cname;
} Mutex;

//////////

typedef struct Type {
  int type;
  bool nullable;
  bool npt;
  bool allow_math;
  bool is_pointer;
  bool is_array;
  bool is_float;
  bool is_unsigned;
  bool c_static;
  bool c_inline;
  //
  int array_size;
  short bytes;
  Class* class;
  Enum* enu;
  // Func ref
  Array* func_arg_types;
  struct Type* func_return_type;
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
  type_wildcard_args,
} TypeType;

typedef struct Value {
  int type;
  Type* return_type;
  void* item;
} Value;
typedef enum ValueType {
  vt_unknown,
  vt_null,
  vt_true,
  vt_false,
  vt_group,
  vt_number,  // 5
  vt_string,
  vt_char,
  vt_operator,
  vt_var,
  vt_func_call,  // 10
  vt_sizeof,
  vt_cast,
  vt_getptr,
  vt_getptrv,
  vt_setptrv,  // 15
  vt_class_init,
  vt_prop_access,
  vt_async,
  vt_await,
  vt_allocator,  // 20
  vt_get_threaded,
  vt_mutex,
} ValueType;

typedef struct ValueFuncCall {
  Array* arg_values;
  Value* on;
} ValueFuncCall;

typedef struct ValueOperator {
  int type;
  Value* left;
  Value* right;
} ValueOperator;

typedef enum OperatorType {
  op_sub,
  op_add,
  op_mult,
  op_div,
  op_mod,
  op_eq,  // 5
  op_neq,
  op_lte,
  op_gte,
  op_lt,
  op_gt,  // 10
  op_incr,
  op_decr,
  op_bit_OR,
  op_bit_AND,
  op_bit_XOR,  // 15
  op_and,
  op_or,
} OperatorType;

typedef struct ValueCast {
  Value* value;
  Type* as_type;
} ValueCast;

typedef struct SetPtrValue {
  Value* ptr_value;
  Value* to_value;
} SetPtrValue;

typedef struct ValueClassInit {
  Class* class;
  Map* prop_values;
} ValueClassInit;

typedef struct ValueClassPropAccess {
  void* on;
  char* name;
  bool is_static;
} ValueClassPropAccess;

//////////

typedef struct Token {
  int type;
  void* item;
} Token;

typedef enum TokenTypeEnum {
  tkn_unknown,
  tkn_return,
  tkn_while,
  tkn_if,
  tkn_throw,
  tkn_break,  // 5
  tkn_continue,
  tkn_free,
  tkn_value,
  tkn_declare,
  tkn_assign,
  tkn_static,
  tkn_set_threaded,
  tkn_mutex_init,
  tkn_mutex_lock,
  tkn_mutex_unlock,
  tkn_task_suspend,
  // Global ast only
  tkn_func,
  tkn_class,
  //
} TokenTypeEnum;

typedef struct TokenIf {
  Value* condition;
  bool is_else;
  struct TokenIf* next;
  struct Scope* scope;
} TokenIf;

typedef struct TokenWhile {
  Value* condition;
  struct Scope* scope;
} TokenWhile;

typedef struct TokenStaticDeclare {
  char* name;
  char* global_name;
  struct Scope* scope;
  struct Type* type;
} TokenStaticDeclare;
typedef struct TokenDeclare {
  char* name;
  struct Value* value;
  struct Type* type;
} TokenDeclare;
typedef struct TokenAssign {
  int type;
  struct Value* left;
  struct Value* right;
} TokenAssign;

typedef struct TokenThrow {
  char* msg;
  Type* return_type;
} TokenThrow;

typedef struct VarInfo {
  char* name;
  Type* return_type;
} VarInfo;

typedef struct TokenIdValue {
  char* name;
  Value* value;
} TokenIdValue;