
#include "../all.h"

typedef struct PkgCompiler {
  char* name;
  char* dir;
  struct Map* namespaces;
  struct Map* package_dirs;
  struct Map* namespace_dirs;
  struct Map* file_compilers;
} PkgCompiler;

typedef struct NsCompiler {
  char* name;
  struct PkgCompiler* pkc;
  struct Scope* scope;
} NsCompiler;

typedef struct FileCompiler {
  struct NsCompiler* nsc;
  //
  char* ki_filepath;
  char* c_filepath;
  char* h_filepath;
  char* o_filepath;
  //
  char* content;
  int content_len;
  //
  int i;
  int line;
  int col;
  // Write c
  Str* c_code;
  Str* c_code_after;
  Str* struct_code;  // Structs
  Str* h_code;       // Function/Vars
  Str* tkn_buffer;
  Str* before_tkn_buffer;
  int indent;
  // Local identifiers
  struct Scope* scope;
  struct Map* uses;
  struct Map* c_imports;
  // Things to compile in this file
  bool create_o_file;
  struct Array* functions;
  struct Array* classes;
  struct Array* enums;
} FileCompiler;

typedef struct Scope {
  struct Map* identifiers;
  bool is_func;
  bool in_loop;
  bool must_return;
  bool did_return;
  bool catch_errors;
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
  idfor_type, // 5
  idfor_property,
} IdentifierForType;

//////////

typedef struct CImport {
  //
  Map* targets;
  struct CImportTarget* target;
  //
  Map* macro_defines;
  //
  struct Map* identifiers;
  struct Map* classes;
  struct Map* functions;
} CImport;

typedef struct CImportTarget {
  char* path;
  bool is_syslib;
  //
  char* content;
  int content_len;
  int i;
  Array* chunks;
  //
  Array* if_results;
  int if_depth;
} CImportTarget;

typedef struct ContentChunk {
  char* content;
  int length;
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
  bool is_number;
  bool is_float;
  bool is_unsigned;
  int size;
  //
  Map* props;
  //
  int body_i;
  int body_i_end;
} Class;

typedef struct ClassProp {
  int access_type;
  bool is_static;
  bool is_func;
  struct Type* return_type;
  struct Value* default_value;
  int value_i;
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

typedef struct Enum {
  char* cname;
  char* name;
  struct Map* values;
} Enum;

//////////

typedef struct Type {
  int type;
  bool nullable;
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
  Function* func;
  Enum* enu;
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
  vt_number, // 5
  vt_string,
  vt_char,
  vt_operator,
  vt_var,
  vt_func_call, // 10
  vt_sizeof,
  vt_cast,
  vt_class_init,
  vt_prop_access,
  // vt_enum_value, // 15
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
  op_eq, // 5
  op_neq,
  op_lte,
  op_gte,
  op_lt,
  op_gt, // 10
  op_incr,
  op_decr,
} OperatorType;

typedef struct ValueCast {
  Value* value;
  Type* as_type;
} ValueCast;

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

typedef enum TokenType {
  tkn_unknown,
  tkn_return,
  tkn_while,
  tkn_if,
  tkn_throw,
  tkn_break, // 5
  tkn_continue,
  tkn_free,
  tkn_value,
  tkn_declare,
  tkn_assign,
  // Global ast only
  tkn_func,
  tkn_class,
} TokenType;

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

typedef struct TokenDeclare {
  char* name;
  struct Value* value;

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