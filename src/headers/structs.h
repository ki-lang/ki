
#ifndef _H_STRUCTS
#define _H_STRUCTS

typedef struct Chain Chain;
typedef struct ChainItem ChainItem;
typedef struct Build Build;
typedef struct Fc Fc;
typedef struct Nsc Nsc;
typedef struct Pkc Pkc;
typedef struct Config Config;
typedef struct Link Link;
typedef struct Chunk Chunk;
typedef struct Scope Scope;
typedef struct VScope VScope;
typedef struct MacroScope MacroScope;
typedef struct Id Id;
typedef struct Idf Idf;
typedef struct Type Type;
typedef struct Class Class;
typedef struct ClassProp ClassProp;
typedef struct Func Func;
typedef struct Enum Enum;
typedef struct Decl Decl;
typedef struct DeclOverwrite DeclOverwrite;
typedef struct Var Var;
typedef struct Arg Arg;
typedef struct UsageLine UsageLine;
typedef struct Global Global;
typedef struct Trait Trait;
typedef struct Alias Alias;
typedef struct TypeCheck TypeCheck;

// Pkg
typedef struct PkgCmd PkgCmd;
typedef struct GithubPkg GithubPkg;
typedef struct PkgVersion PkgVersion;

#include "token.h"
#include "value.h"

struct Allocator {
    AllocatorBlock *first_block;
    AllocatorBlock *current_block;
    AllocatorBlock *last_block;
};
struct AllocatorBlock {
    AllocatorBlock *prev_block;
    AllocatorBlock *next_block;
    size_t size;
    size_t space_left;
    void *start_adr;
    void *current_adr;
    bool private;
};
struct Chain {
    Allocator *alc;
    ChainItem *first;
    ChainItem *last;
    ChainItem *current;
};
struct ChainItem {
    Fc *item;
    ChainItem *next;
};

struct Build {
    int host_os;
    int host_arch;
    int target_os;
    int target_arch;
    //
    char *os;
    char *arch;
    //
    char *path_out;
    char *cache_dir;
    char *token;
    char *sbuf;
    //
    Allocator *alc;
    Allocator *alc_io;
    Allocator *alc_ast;
    //
    Nsc *nsc_main;
    Pkc *pkc_ki;
    Func *main_func;
    Nsc *nsc_type;
    Nsc *nsc_io;
    //
    MacroScope *mc;
    //
    Array *packages;
    Array *all_ki_files;
    Array *link_dirs;
    Map *link_libs;
    Map *all_fcs;
    Str *str_buf;
    Str *str_buf_io;
    //
    Chain *read_ki_file;
    Chain *write_ir;
    Chain *stage_1;
    Chain *stage_2;
    Chain *stage_3;
    Chain *stage_4;
    Chain *stage_5;
    Chain *stage_6;
    //
    Class *class_u8;
    Class *class_u16;
    Class *class_u32;
    Class *class_u64;
    Class *class_i8;
    Class *class_i16;
    Class *class_i32;
    Class *class_i64;
    Class *class_ptr;
    Class *class_string;
    Class *class_array;
    Class *class_map;
    //
    int event_count;
    int events_done;
    int ptr_size;
    int verbose;
    int LOC;
    //
    bool ir_ready;
    bool optimize;
    bool test;
    bool debug;
    bool clear_cache;
    bool run_code;
    bool core_types_scanned;
    bool link_static;
};

struct Fc {
    Build *b;
    char *path_ki;
    char *path_ir;
    char *path_cache;
    char *token;
    char *sbuf;
    char *ir;
    char *ir_hash;
    Id *id_buf;
    Nsc *nsc;
    Pkc *pkc_config;
    Allocator *alc;
    Allocator *alc_ast;
    Array *deps;
    Array *sub_headers;
    Chunk *chunk;
    Chunk *chunk_prev;
    Scope *scope;
    MacroScope *current_macro_scope;
    //
    Func *error_func_info;
    Class *error_class_info;
    //
    Array *funcs;
    Array *classes;
    Array *globals;
    Array *aliasses;
    Array *class_size_checks;
    Array *type_size_checks;
    //
    cJSON *cache;
    //
    bool is_header;
    bool ir_changed;
    bool generated;
};

struct Nsc {
    Build *b;
    Pkc *pkc;
    char *name;
    char *path_o;
    Array *fcs;
    Scope *scope;
};

struct Pkc {
    Build *b;
    char *name;
    char *dir;
    char *hash;
    Map *sub_packages;
    Map *namespaces;
    Config *config;
    Array *header_dirs;
};
struct Config {
    char *path;
    char *dir;
    char *content;
    cJSON *json;
};

struct Link {
    int type;
};

struct Chunk {
    Fc *fc;
    char *content;
    int length;
    int i;
    int line;
};
struct Scope {
    int type;
    Scope *parent;
    Map *identifiers;
    Map *upref_slots;
    Func *func;
    Array *ast;
    Array *defer_ast;
    Array *usage_keys;
    Array *usage_values;
    VScope *vscope;
    bool did_return;
    bool did_exit_function;
    bool in_loop;
};
struct VScope {
    Type *rett;
    char *lvar;
    void *llvm_after_block;
};
struct MacroScope {
    Map *identifiers;
    MacroScope *parent;
};
struct Id {
    char *nsc_name;
    char *name;
    bool has_nsc;
};
struct Idf {
    void *item;
    int type;
};

struct Type {
    Class *class;
    Enum *enu;
    Array *func_args;
    Type *func_rett;
    Array *func_errors;
    Type *array_of;
    int type;
    int bytes;
    int ptr_depth;
    int array_size;
    bool is_signed;
    bool nullable;
    bool func_can_error;
    bool strict_ownership;
    bool borrow;
    bool ref;
    bool weak_ptr;
};

struct Class {
    char *name;
    char *gname;
    char *dname;
    Fc *fc;
    Scope *scope;
    Chunk *chunk_body;
    Map *props;
    Map *funcs;
    Func *func_ref;
    Func *func_deref;
    Func *func_deref_props;
    Func *func_free;
    Func *func_iter_init;
    Func *func_iter_get;
    Func *func_before_free;
    Func *func_leave_scope;
    Array *generic_names;
    Array *generic_types;
    Map *generics;
    int type;
    int size;
    bool is_rc;
    bool is_signed;
    bool packed;
    bool is_generic_base;
    bool allow_math;
    bool track_ownership;
    bool must_ref;
    bool must_deref;
    bool is_struct;
    bool can_iter;
    bool async;
    bool has_borrows;
};
struct ClassProp {
    Type *type;
    Value *value;
    Chunk *value_chunk;
    int index;
    int act;
};
struct Func {
    char *name;
    char *gname;
    char *dname;
    Fc *fc;
    Scope *scope;
    Chunk *chunk_args;
    Chunk *chunk_body;
    Type *rett;
    Array *args;
    Map *args_by_name;
    Array *errors;
    //
    int act; // Access type for class functions
    //
    bool is_static;
    bool is_generated;
    bool can_error;
    bool only_returns_strict;
    bool will_exit;
    bool uses_stack_alloc;
    // Optimizations
    bool opt_hot;
    bool opt_inline;
};
struct Enum {
    char *name;
    char *gname;
    char *dname;
    Fc *fc;
    Map *values;
};

struct Decl {
    char *name;
    Type *type;
    Value *value;
    Scope *scope;
    char *llvm_val;
    bool is_mut;
    bool is_arg;
};
struct DeclOverwrite {
    Type *type;
    Decl *decl;
};
struct Var {
    Decl *decl;
    Type *type;
};
struct Arg {
    char *name;
    Type *type;
    Value *value;
    Chunk *value_chunk;
    Chunk *type_chunk;
    Decl *decl;
    bool is_mut;
};
struct UsageLine {
    Decl *decl;
    Scope *scope;
    Chunk *first_move;
    ValueAndExec *upref_token;
    TExec *deref_token;
    Array *ancestors;
    UsageLine *parent;
    UsageLine *clone_from;
    Scope *deref_scope;
    int moves;
    int moves_possible;
    int reads_after_move;
    bool read_after_move;
    bool enable;
    bool first_in_scope;
};

struct Global {
    char *name;
    char *gname;
    char *dname;
    Fc *fc;
    Type *type;
    Chunk *type_chunk;
    bool shared;
};

struct Trait {
    char *name;
    char *gname;
    char *dname;
    Fc *fc;
    Chunk *chunk;
};

struct Alias {
    Chunk *chunk;
    char *name;
    int type;
};

struct TypeCheck {
    Class *class;
    TypeCheck *array_of;
    int array_size;
    int type;
    bool borrow;
    bool ref;
};

// Pkg
struct PkgCmd {
    Allocator *alc;
    char *char_buf;
    Str *str_buf;
};

struct GithubPkg {
    char *url;
    char *username;
    char *pkgname;
};

struct PkgVersion {
    int v1;
    int v2;
    int v3;
};

#endif
