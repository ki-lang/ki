
#include "../all.h"

char *default_os();
char *default_arch();
void cmd_build_help();
void build_add_files(Build *b, Array *files);
char *find_config_dir(Allocator *alc, char *ki_path);

void cmd_build(int argc, char *argv[]) {
    //
    Allocator *alc = alc_make();
    Allocator *alc_io = alc_make();
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
    if (array_contains(args, "-h", arr_find_str) || !path_out || strlen(path_out) == 0) {
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
    if (array_contains(args, "-v", arr_find_str)) {
        verbose = 1;
    }
    if (array_contains(args, "-vv", arr_find_str)) {
        verbose = 2;
    }
    if (array_contains(args, "-vvv", arr_find_str)) {
        verbose = 3;
    }

    bool optimize = array_contains(args, "--optimize", arr_find_str) || array_contains(args, "-O", arr_find_str);
    bool debug = array_contains(args, "--debug", arr_find_str) || array_contains(args, "-d", arr_find_str);
    bool test = array_contains(args, "--test", arr_find_str);

    //
    Build *b = al(alc, sizeof(Build));
    b->token = al(alc, KI_TOKEN_MAX);
    b->sbuf = al(alc, 2000);

    // Filter out files
    char *first_file = NULL;
    Array *files = array_make(alc, argc);
    argc = args->length;
    for (int i = 2; i < argc; i++) {
        char *arg = array_get_index(args, i);
        if (arg[0] == '-') {
            continue;
        }
        if (!ends_with(arg, ".ki")) {
            sprintf(b->sbuf, "Filename must end with .ki : '%s'", arg);
            die(b->sbuf);
        }

        char *full = al(alc, KI_PATH_MAX);
        bool success = get_fullpath(arg, full);

        if (!success || !file_exists(full)) {
            sprintf(b->sbuf, "File not found: '%s'", arg);
            die(b->sbuf);
        }

        if (!first_file)
            first_file = full;

        array_push(files, full);
    }

    if (!first_file) {
        sprintf(b->sbuf, "Nothing to compile, add some files to your command");
        die(b->sbuf);
    }

    // Cache dir
    char *cache_buf = malloc(1000);
    char *cache_hash = malloc(64);
    char *cache_dir = al(alc, KI_PATH_MAX);
    get_dir_from_path(first_file, cache_buf);
    strcat(cache_buf, "||");
    strcat(cache_buf, optimize ? "1" : "0");
    strcat(cache_buf, debug ? "1" : "0");
    strcat(cache_buf, test ? "1" : "0");
    md5(cache_buf, cache_hash);
    free(cache_buf);
    strcpy(cache_dir, get_storage_path());
    strcat(cache_dir, "/cache/");

    if (!file_exists(cache_dir)) {
        makedir(cache_dir, 0700);
    }

    strcat(cache_dir, cache_hash);
    free(cache_hash);

    if (!file_exists(cache_dir)) {
        makedir(cache_dir, 0700);
    }

    if (verbose > 0) {
        printf("📦 Cache dir : %s\n", cache_dir);
        printf("💿 Target : %s-%s\n", os, arch);
    }

    //
    b->os = os;
    b->arch = arch;
    b->path_out = path_out;
    b->ptr_size = ptr_size;
    b->alc = alc;
    b->alc_io = alc_io;
    b->alc_ast = alc_make();
    b->cache_dir = cache_dir;
    //
    b->event_count = 0;
    b->events_done = 0;
    b->verbose = verbose;
    //
    b->packages = array_make(alc, 100);
    b->all_ki_files = array_make(alc, 1000);
    b->link_libs = array_make(alc, 160);
    b->link_dirs = array_make(alc, 40);
    b->main_func = NULL;
    b->str_buf = str_make(alc, 5000);
    b->str_buf_io = str_make(alc_io, 10000);
    //
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
    b->optimize = optimize;
    b->test = test;
    b->debug = debug;

    Pkc *pkc_main = pkc_init(alc, b, "main", find_config_dir(alc, first_file));
    Nsc *nsc_main = nsc_init(alc, b, pkc_main, "main");

    char *ki_dir = al(alc, KI_PATH_MAX);
    strcpy(ki_dir, get_binary_dir());
    strcat(ki_dir, "/lib");
    Pkc *pkc_ki = pkc_init(alc, b, "ki", ki_dir);

    b->nsc_main = nsc_main;
    b->pkc_ki = pkc_ki;

    array_push(b->packages, pkc_ki);
    array_push(b->packages, pkc_main);

    pkc_load_nsc(pkc_ki, "type", NULL);
    pkc_load_nsc(pkc_ki, "mem", NULL);
    pkc_load_nsc(pkc_ki, "io", NULL);

    //
    pthread_t thr;
    pthread_create(&thr, NULL, io_loop, (void *)b);

    // Compile ki lib
    compile_loop(b, 5);

    // Compile CLI files
    build_add_files(b, files);
    compile_loop(b, 6);

    b->ir_ready = true;

    printf("🔗 Link executable\n");

    stage_8(b);

    printf("✅ Done\n");
}

void build_add_files(Build *b, Array *files) {
    //
    int filec = files->length;
    for (int i = 0; i < filec; i++) {
        char *path = array_get_index(files, i);
        Fc *fc = fc_init(b, path, b->nsc_main, false);
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

char *find_config_dir(Allocator *alc, char *ki_path) {
    //
    char *dir = al(alc, KI_PATH_MAX);
    get_dir_from_path(ki_path, dir);
    char cfg_path[strlen(dir) + 10];

    bool found = false;
    while (strlen(dir) > 3) {
        strcpy(cfg_path, dir);
        strcat(cfg_path, "ki.json");
        if (file_exists(cfg_path)) {
            dir[strlen(dir) - 1] = '\0';
            found = true;
            break;
        }
        get_dir_from_path(dir, dir);
    }
    if (!found) {
        get_dir_from_path(ki_path, dir);
        dir[strlen(dir) - 1] = '\0';
    }
    return dir;
}

Class *ki_get_class(Build *b, char *ns, char *name) {
    //
    Pkc *pkc = b->pkc_ki;
    Nsc *nsc = pkc_get_nsc(pkc, ns);

    Idf *idf = map_get(nsc->scope->identifiers, name);
    if (!idf || idf->type != idf_class) {
        sprintf(b->sbuf, "Class not found in ki-lib '%s:%s'", ns, name);
        die(b->sbuf);
    }

    return idf->item;
}

Func *ki_get_func(Build *b, char *ns, char *name) {
    //
    Pkc *pkc = b->pkc_ki;
    Nsc *nsc = pkc_get_nsc(pkc, ns);

    Idf *idf = map_get(nsc->scope->identifiers, name);
    if (!idf || idf->type != idf_func) {
        sprintf(b->sbuf, "Func not found in ki-lib '%s:%s'", ns, name);
        die(b->sbuf);
    }

    return idf->item;
}
