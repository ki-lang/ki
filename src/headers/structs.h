
typedef struct Build Build;
typedef struct Fc Fc;
typedef struct Nsc Nsc;
typedef struct Pkc Pkc;

struct Build {
    char *os;
    char *arch;
    //
    char *cache_dir;
    //
    Array *allocs;
    //
    int ptr_size;
};

struct Fc {
    //
    char *path_ki;
    char *path_ir;
    //
    Array *allocs;
};

struct Nsc {
    //
    char *path_o;
    //
    Array *fcs;
};

struct Pkc {
    //
    Map *namespaces;
};
