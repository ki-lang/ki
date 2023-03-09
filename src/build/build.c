
#include "../all.h"

char *default_os();
char *default_arch();
void cmd_build_help();
void build_add_files(Build *b, Array *files);

void cmd_build(int argc, char *argv[]) {
    //
    Allocator *alc = alc_make();
    //
    Array *args = array_make(alc, argc);
    Map *options = map_make(alc);
    Array *has_value = array_make(alc, 8);
    array_push(has_value, "-o");
    array_push(has_value, "--arch");
    array_push(has_value, "--os");

    parse_argv(argv, argc, has_value, args, options);

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

    int verbose = 0;
    ;
    if (array_contains(args, "-v", "chars")) {
        verbose = 1;
    }
    if (array_contains(args, "-vv", "chars")) {
        verbose = 2;
    }
    if (array_contains(args, "-vvv", "chars")) {
        verbose = 3;
    }

    //
    Build *b = al(alc, sizeof(Build));
    b->msg = al(alc, 2000);

    // Filter out files
    Array *files = array_make(alc, argc);
    argc = args->length;
    for (int i = 2; i < argc; i++) {
        char *arg = array_get_index(args, i);
        if (arg[0] == '-') {
            continue;
        }
        if (!ends_with(arg, ".ki")) {
            sprintf(b->msg, "Filename must end with .ki : '%s'", arg);
            die(b->msg);
        }
        array_push(files, arg);
    }

    //
    b->os = os;
    b->arch = arch;
    b->ptr_size = ptr_size;
    b->alc = alc;
    b->alc_ast = alc_make();
    //
    b->event_count = 0;
    b->events_done = 0;
    b->verbose = verbose;
    //
    b->all_ki_files = array_make(alc, 1000);
    b->read_ki_file = chain_make(alc);
    b->write_ir = chain_make(alc);
    b->stage_1 = chain_make(alc);
    b->stage_2 = chain_make(alc);
    b->stage_3 = chain_make(alc);
    b->stage_4 = chain_make(alc);
    b->stage_5 = chain_make(alc);
    b->stage_6 = chain_make(alc);
    //
    b->ir_ready = false;

    Pkc *pkc_main = al(alc, sizeof(Pkc));
    Nsc *nsc_main = al(alc, sizeof(Nsc));
    nsc_main->pkc = pkc_main;
    nsc_main->b = b;

    b->nsc_main = nsc_main;

    //
    pthread_t thr;
    pthread_create(&thr, NULL, io_loop, (void *)b);

    // Compile ki lib
    compile_loop(b, 5);

    // Compile CLI files
    build_add_files(b, files);
    compile_loop(b, 6);

    printf("# Link\n");

    printf("# Done\n");
}

void build_add_files(Build *b, Array *files) {
    //
    int filec = files->length;
    for (int i = 0; i < filec; i++) {
        char *path = array_get_index(files, i);
        char *fpath = get_fullpath(path);
        Fc *fc = fc_init(b, fpath, b->nsc_main);
    }
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

void cmd_build_help() {
    //
    printf("\n# ki build {ki-files} -o {outpath}\n\n");
    printf(" --os            compile for target OS: linux, win, macos\n");
    printf(" --arch          compile for target arch: x86, x64, arm64\n");
    printf("\n");
    exit(1);
}
