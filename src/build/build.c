
#include "../all.h"

char *default_os();
char *default_arch();
void cmd_build_help();
void build_add_files(Build *b, Array *files);
void build_start(Build *b);
void build_free(Build *b);

void cmd_build(int argc, char *argv[]) {
    //
    Array *args = array_make(argc);
    Map *options = map_make();
    Array *has_value = array_make(8);
    array_push(has_value, "-o");
    array_push(has_value, "--arch");
    array_push(has_value, "--os");

    parse_argv(argv, argc, has_value, args, options);
    array_free(has_value, false);

    // Check options
    char *path_out = map_get(options, "-o");
    if (!path_out || array_contains(args, "-h", "chars")) {
        cmd_build_help();
    }

    char *os = map_get(options, "--os");
    char *arch = map_get(options, "--arch");
    if (!os)
        os = default_os();
    if (!arch)
        arch = default_arch();

    int ptr_size = 8;
    if (strcmp(arch, "x86") == 0) {
        ptr_size = 4;
    }

    // Filter out files
    Array *files = array_make(argc);
    argc = args->length;
    for (int i = 2; i < argc; i++) {
        char *arg = array_get_index(args, i);
        if (arg[0] == '-') {
            continue;
        }
        if (!ends_with(arg, ".ki")) {
            sprintf(die_buf, "Filename must end with .ki : '%s'", arg);
            die(die_buf);
        }
        array_push(files, arg);
    }
    //
    Build *b = malloc(sizeof(Build));
    b->os = os;
    b->arch = arch;
    b->ptr_size = ptr_size;
    b->allocs = array_make(100);

    Pkc *pkc_main = b_alloc(b, sizeof(Pkc));
    Nsc *nsc_main = b_alloc(b, sizeof(Nsc));
    nsc_main->pkc = pkc_main;
    nsc_main->b = b;

    b->nsc_main = nsc_main;

    build_add_files(b, files);
    build_start(b);

    //
    build_free(b);
    map_free(options, false);
    array_free(args, false);
    array_free(files, false);
}

void build_add_files(Build *b, Array *files) {
    //
    int filec = files->length;
    for (int i = 0; i < filec; i++) {
        char *path = array_get_index(files, i);
        char *fpath = get_fullpath(path);
        Fc *fc = fc_init(b, fpath);
    }
}
void build_start(Build *b) {
    //
}

char *default_os() {
//
#if _WIN32
    return "win";
#endif
#if __unix__
#if __linux__
    return "linux";
#else
    return "bsd";
#endif
#endif
#if __APPLE__
    return "macos";
#endif
    die("Cannot determine default target 'os', use --os to specify manually");
}

char *default_arch() {
//
#if defined(__x86_64__) || defined(_M_X64)
    return "x64";
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
    return "x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
#endif
    die("Cannot determine default target 'arch', use --arch to specify manually");
}

void build_free(Build *b) {
    //
    Array *allocs = b->allocs;
    int alc = b->allocs->length;
    for (int i = 0; i < alc; i++) {
        free(array_get_index(allocs, i));
    }
    array_free(b->allocs, false);
    free(b);
}

void cmd_build_help() {
    //
    printf("\n# ki build {ki-files} -o {outpath}\n\n");
    printf(" --os            compile for target OS: linux, win, macos\n");
    printf(" --arch          compile for target arch: x86, x64, arm64\n");
    printf("\n");
    exit(1);
}
