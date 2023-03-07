
typedef struct Build Build;
typedef struct Fc Fc;

struct Build {
    char *os;
    char *arch;
    //
    char *cache_dir;
    //
    Array *allocs;
};

struct Fc {
    //
    char *path_ki;
    char *path_ir;
    //
    Array *allocs;
};
