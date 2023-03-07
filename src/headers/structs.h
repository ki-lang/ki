
typedef struct Build Build;
typedef struct Fc Fc;
typedef struct Nsc Nsc;
typedef struct Pkc Pkc;

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
