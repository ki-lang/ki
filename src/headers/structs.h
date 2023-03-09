
typedef struct Chain Chain;
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
struct Chain {
    Fc *first;
    Fc *last;
    Fc *current;
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
