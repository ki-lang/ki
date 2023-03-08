
typedef struct Allocator Allocator;
typedef struct AllocatorBlock AllocatorBlock;
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
    char *cache_dir;
    Array *allocs;
    Nsc *nsc_main;
    //
    int ptr_size;
};

struct Fc {
    Build *b;
    char *path_ki;
    char *path_ir;
    Array *allocs;
    Nsc *nsc;
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
