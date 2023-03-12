
#ifndef _H_STRUCTS
#define _H_STRUCTS

typedef struct Chain Chain;
typedef struct ChainItem ChainItem;
typedef struct Build Build;
typedef struct Fc Fc;
typedef struct Nsc Nsc;
typedef struct Pkc Pkc;
typedef struct Config Config;
typedef struct Chunk Chunk;
typedef struct Scope Scope;
typedef struct MacroScope MacroScope;
typedef struct Id Id;
typedef struct Idf Idf;
typedef struct Type Type;
typedef struct Class Class;
typedef struct ClassProp ClassProp;
typedef struct Func Func;
typedef struct Enum Enum;
typedef struct Var Var;
typedef struct Arg Arg;

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
    char *os;
    char *arch;
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
    //
    Array *packages;
    Array *all_ki_files;
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
    int event_count;
    int events_done;
    int ptr_size;
    int verbose;
    //
    bool ir_ready;
    bool optimize;
    bool test;
    bool debug;
};

struct Fc {
    Build *b;
    char *path_ki;
    char *path_ir;
    char *token;
    char *sbuf;
    char *ir_hash;
    Id *id_buf;
    Nsc *nsc;
    Allocator *alc;
    Allocator *alc_ast;
    Array *deps;
    Str *ir;
    Chunk *chunk;
    Chunk *chunk_prev;
    Scope *scope;
    MacroScope *current_macro_scope;
    //
    Array *funcs;
    Array *classes;
    Array *class_size_checks;
    Array *type_size_checks;
    //
    bool is_header;
    bool ir_changed;
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
    Map *namespaces;
    Config *config;
};
struct Config {
    char *path;
    char *content;
    cJSON *json;
};

struct Chunk {
    char *content;
    int length;
    int i;
    int line;
};
struct Scope {
    int type;
    Scope *parent;
    Map *identifiers;
    Func *func;
    Array *ast;
    Map *lvars; // LLVM vars
    bool did_return;
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
    int type;
    int bytes;
    int ptr_depth;
    bool is_signed;
    bool nullable;
    bool func_can_error;
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
    int type;
    int size;
    bool is_rc;
    bool is_signed;
    bool packed;
    bool is_generic_base;
};
struct ClassProp {
    Type *type;
    Value *value;
    Chunk *value_chunk;
    int index;
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
};
struct Enum {
    char *name;
    char *gname;
    char *dname;
    Fc *fc;
};

struct Var {
    char *name;
    Type *type;
    bool is_mut;
    bool is_global;
    bool is_arg;
};
struct Arg {
    char *name;
    Type *type;
    bool is_mut;
    Value *value;
    Chunk *value_chunk;
};

#endif
