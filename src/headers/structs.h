
typedef struct Build Build;
typedef struct Fc Fc;
typedef struct Nsc Nsc;
typedef struct Pkc Pkc;

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

struct Build {
    char *os;
    char *arch;
    //
    Allocator *alc;
    Allocator *alc_ast;
    //
    char *cache_dir;
    Nsc *nsc_main;
    //
    Array *all_ki_files;
    Array *queue_read_ki_file;
    Array *queue_write_ir;
    Array *stage_1;
    Array *stage_2;
    Array *stage_3;
    Array *stage_4;
    Array *stage_5;
    Array *stage_6;
    //
    int ptr_size;
    //
    bool ir_ready;
};

struct Fc {
    Build *b;
    char *path_ki;
    char *path_ir;
    char *content;
    Nsc *nsc;
    Allocator *alc;
    Allocator *alc_ast;
    Array *deps;
    int stage;
    Str *ir;
};

struct Nsc {
    Build *b;
    Pkc *pkc;
    char *path_o;
    Array *fcs;
};

struct Pkc {
    Build *b;
    Map *namespaces;
};
