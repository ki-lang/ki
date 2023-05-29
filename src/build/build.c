
#include "../all.h"

int default_os();
int default_arch();
void cmd_build_help(bool run_code);
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
    array_push(has_value, "--target");

    parse_argv(argv, argc, has_value, args, options);

    // Args
    char argbuf[256];
    for (int i = 0; i < args->length; i++) {
        char *arg = array_get_index(args, i);
        if (arg[0] != '-')
            continue;
        sprintf(argbuf, ".%s.", arg);
        if (!strstr(".--options.-O.--debug.-d.--test.--clean.-c.--static.-s.--run.-r.-h.--help.-v.-vv.-vvv.", argbuf)) {
            sprintf(argbuf, "❓ Unknown option '%s'", arg);
            die(argbuf);
        }
    }

    bool run_code = strcmp(argv[1], "run") == 0 || array_contains(args, "--run", arr_find_str) || array_contains(args, "-r", arr_find_str);

    // Check options
    char *path_out = map_get(options, "-o");
    if (array_contains(args, "-h", arr_find_str) || array_contains(args, "--help", arr_find_str) || (!run_code && !path_out) || (path_out && strlen(path_out) == 0)) {
        cmd_build_help(run_code);
    }

    int host_os = default_os();
    int host_arch = default_arch();
    int target_os = host_os;
    int target_arch = host_arch;

    char *target = map_get(options, "--target");

    if (target) {
        if (strcmp(target, "linux-x64") == 0) {
            target_os = os_linux;
            target_arch = arch_x64;
        } else if (strcmp(target, "linux-arm64") == 0) {
            target_os = os_linux;
            target_arch = arch_arm64;
        } else if (strcmp(target, "macos-x64") == 0) {
            target_os = os_macos;
            target_arch = arch_x64;
        } else if (strcmp(target, "macos-arm64") == 0) {
            target_os = os_macos;
            target_arch = arch_arm64;
        } else if (strcmp(target, "win-x64") == 0) {
            target_os = os_win;
            target_arch = arch_x64;
        } else if (strcmp(target, "win-arm64") == 0) {
            target_os = os_win;
            target_arch = arch_arm64;
        } else {
            char err[256];
            sprintf(err, "Unsupported target: '%s' | options:\n - linux-x64\n - linux-arm64\n - macos-x64\n - macos-arm64\n - win-x64\n - win-arm64\n\n", target);
            die(err);
        }
    }

    char *os = NULL;
    char *arch = NULL;

    if (target_os == os_linux)
        os = "linux";
    else if (target_os == os_macos)
        os = "macos";
    else if (target_os == os_win)
        os = "win";

    if (target_arch == arch_x64)
        arch = "x64";
    else if (target_arch == arch_arm64)
        arch = "arm64";

    int ptr_size = 8;
    int verbose = 0;

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
    bool clear_cache = array_contains(args, "--clean", arr_find_str) || array_contains(args, "-c", arr_find_str);
    bool link_static = array_contains(args, "--static", arr_find_str) || array_contains(args, "-s", arr_find_str);

    //
    Build *b = al(alc, sizeof(Build));
    b->token = al(alc, KI_TOKEN_MAX);
    b->sbuf = al(alc, 2000);

    MacroScope *mc = init_macro_scope(alc);
    map_set(mc->identifiers, "OS", os);
    map_set(mc->identifiers, "ARCH", arch);
    b->mc = mc;

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
    strcat(cache_buf, os);
    strcat(cache_buf, arch);
    strcat(cache_buf, optimize ? "1" : "0");
    strcat(cache_buf, debug ? "1" : "0");
    strcat(cache_buf, test ? "1" : "0");
    simple_hash(cache_buf, cache_hash);
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

    if (!path_out) {
        path_out = al(alc, KI_PATH_MAX);
        strcpy(path_out, cache_dir);
        strcat(path_out, "/out");
        if (verbose > 0) {
            printf("💾 Temporary out : %s\n", path_out);
        }
    }

    //
    b->host_os = host_os;
    b->host_arch = host_arch;
    b->target_os = target_os;
    b->target_arch = target_arch;
    //
    b->os = os;
    b->arch = arch;
    //
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
    b->packages_by_dir = map_make(alc);
    b->all_ki_files = array_make(alc, 1000);
    b->link_libs = map_make(alc);
    b->link_dirs = array_make(alc, 40);
    b->all_fcs = map_make(alc);
    b->main_func = NULL;
    b->str_buf = str_make(alc, 5000);
    b->str_buf_io = str_make(alc_io, 10000);
    b->tests = array_make(alc, 20);
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
    b->class_u8 = NULL;
    b->class_u16 = NULL;
    b->class_u32 = NULL;
    b->class_u64 = NULL;
    b->class_i8 = NULL;
    b->class_i16 = NULL;
    b->class_i32 = NULL;
    b->class_i64 = NULL;
    b->class_string = NULL;
    b->class_array = NULL;
    b->class_ptr = NULL;
    //
    b->core_types_scanned = false;
    b->ir_ready = false;
    b->optimize = optimize;
    b->test = test;
    b->debug = debug;
    b->clear_cache = clear_cache;
    b->run_code = run_code;
    b->LOC = 0;
    b->link_static = link_static;
    //

#ifdef WIN32
    LARGE_INTEGER frequency;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
#else
    struct timeval begin, end;
    gettimeofday(&begin, NULL);
#endif

    Pkc *pkc_main = pkc_init(alc, b, "main", find_config_dir(alc, first_file));
    Nsc *nsc_main = nsc_init(alc, b, pkc_main, "main");

    char *pkg_dir = al(alc, KI_PATH_MAX);
    strcpy(pkg_dir, pkc_main->dir);
    strcat(pkg_dir, "/packages");
    b->pkg_dir = pkg_dir;

    char *ki_dir = al(alc, KI_PATH_MAX);
    strcpy(ki_dir, get_binary_dir());
    strcat(ki_dir, "/lib");
    Pkc *pkc_ki = pkc_init(alc, b, "ki", ki_dir);

    b->nsc_main = nsc_main;
    b->pkc_ki = pkc_ki;

    array_push(b->packages, pkc_ki);
    array_push(b->packages, pkc_main);

    b->nsc_type = pkc_load_nsc(pkc_ki, "type", NULL);
    b->nsc_io = pkc_load_nsc(pkc_ki, "io", NULL);
    pkc_load_nsc(pkc_ki, "mem", NULL);
    pkc_load_nsc(pkc_ki, "os", NULL);

    //
#ifdef WIN32
    void *thr = CreateThread(NULL, 0, (unsigned long (*)(void *))io_loop, (void *)b, 0, NULL);
#else
    pthread_t thr;
    pthread_create(&thr, NULL, io_loop, (void *)b);
#endif

    // Compile ki lib
    compile_loop(b, 5);

    // Compile CLI files
    build_add_files(b, files);
    compile_loop(b, 5); // Get everything to stage 5 first
    compile_loop(b, 6); // Parse AST & generate IR

    b->ir_ready = true;

    if (b->verbose > 0) {
        printf("📄 Lines of code: %d\n", b->LOC);
        printf("🔗 Link executable\n");
        if (b->optimize) {
            printf("✨ Optimize\n");
        }
    }

#ifdef WIN32
    QueryPerformanceCounter(&end);
    double time_ast = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
#else
    gettimeofday(&end, NULL);
    double time_ast = (double)(end.tv_usec - begin.tv_usec) / 1000000 + (double)(end.tv_sec - begin.tv_sec);
#endif

    if (verbose > 0) {
        printf("⌚ Parse + IR gen: %.3fs\n", time_ast);
    }

    stage_8(b);

#ifdef WIN32
    QueryPerformanceCounter(&end);
    double time_all = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
#else
    gettimeofday(&end, NULL);
    double time_all = (double)(end.tv_usec - begin.tv_usec) / 1000000 + (double)(end.tv_sec - begin.tv_sec);
#endif

    if (!run_code || b->verbose > 0) {
        printf("✅ Compiled in: %.3fs\n", time_all);
    }

#ifdef WIN32
    _flushall();
#else
    sync();
#endif
    int i = 0;
    while (!file_exists(b->path_out)) {
        sleep_ms(10);
        i++;
        if (i == 100)
            break;
    }

    if (run_code) {
        system(b->path_out);
    }
}

void build_add_files(Build *b, Array *files) {
    //
    int filec = files->length;
    for (int i = 0; i < filec; i++) {
        char *path = array_get_index(files, i);
        Fc *fc = fc_init(b, path, b->nsc_main, b->nsc_main->pkc, false);
    }
}

int default_os() {
//
#if _WIN32
    return os_win;
#endif
#if __unix__
#if __linux__
    return os_linux;
#else
    // return os_bsd;
#endif
#endif
#if __APPLE__
    return os_macos;
#endif
    die("Cannot determine default target 'os', use --os to specify manually");
}

int default_arch() {
//
#if defined(__x86_64__) || defined(_M_X64)
    return arch_x64;
// #elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
//     return "x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return arch_arm64;
#endif
    die("Cannot determine default target 'arch', use --arch to specify manually");
}

void cmd_build_help(bool run_code) {
    //
    if (run_code)
        printf("\n# ki run {ki-files}\n");
    else
        printf("\n# ki build {ki-files} -o {outpath}\n");
    printf("\n");

    printf(" --optimize -O       apply code optimizations\n");
    printf(" --clean -c          clear cache\n");
    printf(" --debug -d          generate debug info\n");
    printf(" --run -r            run code after compiling\n");
    printf(" --test              generate a 'main' that runs all tests\n");
    printf(" --target            compile for a specific os/arch\n");
    printf("                     linux-x64, macos-x64, win-x64\n");
    printf("\n");

    printf(" -v -vv -vvv         show compile info\n");
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