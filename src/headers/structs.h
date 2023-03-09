
typedef struct Chain Chain;
typedef struct Build Build;
typedef struct Fc Fc;
typedef struct Nsc Nsc;
typedef struct Pkc Pkc;
typedef struct Chunk Chunk;
typedef struct Scope Scope;
typedef struct MacroScope MacroScope;
typedef struct Id Id;
typedef struct Idf Idf;
typedef struct Type Type;
typedef struct Class Class;
typedef struct Func Func;

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
};
struct Chain {
    Fc *first;
    Fc *last;
    Fc *current;
};

struct Build {
    char *os;
    char *arch;
    char *cache_dir;
    char *token;
    char *sbuf;
    //
    Allocator *alc;
    Allocator *alc_ast;
    //
    Nsc *nsc_main;
    Pkc *pkc_ki;
    Func *main_func;
    //
    Array *all_ki_files;
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
};

struct Fc {
    Build *b;
    Fc *next;
    char *path_ki;
    char *path_ir;
    char *token;
    char *sbuf;
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
    Map *namespaces;
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
};
struct MacroScope {
    Map *identifiers;
    MacroScope *parent;
};
struct Id {
    char *nsc_name;
    char *name;
};
struct Idf {
    int type;
    void *item;
};

struct Type {
    Class *class;
    int type;
    bool is_signed;
    bool nullable;
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
    bool is_rc;
    bool is_signed;
};
struct Func {
    char *name;
    char *gname;
    char *dname;
    Fc *fc;
    Scope *scope;
    Chunk *chunk_args;
    Chunk *chunk_body;
};
struct Enum {
    char *name;
    char *gname;
    char *dname;
    Fc *fc;
};
