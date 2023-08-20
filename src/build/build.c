
#include "../all.h"

int default_os();
int default_arch();
void cmd_build_help(bool run_code);
void build_add_files(Build *b, Array *files);
void build_macro_defs(Build *b, char *defs);
void build_watch(Build *b, int argc, char *argv[]);

void cmd_build(int argc, char *argv[], LspData *lsp_data) {
    //
    Allocator *alc = alc_make();
    Allocator *alc_io = alc_make();
    //
    Build *b = al(alc, sizeof(Build));
    b->alc = alc;
    b->alc_io = alc_io;
    b->token = al(alc, KI_TOKEN_MAX);
    b->sbuf = al(alc, 2000);
    b->lsp = lsp_data;
    //
    Array *args = array_make(alc, argc);
    Map *options = map_make(alc);
    Array *has_value = array_make(alc, 8);
    array_push(has_value, "-o");
    array_push(has_value, "--target");
    array_push(has_value, "--def");

    parse_argv(argv, argc, has_value, args, options);

    // Args
    for (int i = 0; i < args->length; i++) {
        char *arg = array_get_index(args, i);
        if (arg[0] != '-')
            continue;
        sprintf(b->sbuf, ".%s.", arg);
        if (!strstr(".--optimize.-O.--debug.-d.--test.--clean.-c.--static.-s.--run.-r.--help.-h.-v.-vv.-vvv.--watch.", b->sbuf)) {
            sprintf(b->sbuf, "❓ Unknown option '%s'", arg);
            build_error(b, b->sbuf);
        }
    }

    bool run_code = strcmp(argv[1], "run") == 0 || array_contains(args, "--run", arr_find_str) || array_contains(args, "-r", arr_find_str);

    // Check options
    char *path_out = map_get(options, "-o");
    if (path_out && path_out[0] == '-') {
        sprintf(b->sbuf, "Invalid value for -o, first character cannot be '-' | Value: '%s'", path_out);
        build_error(b, b->sbuf);
    }

    if (!lsp_data && (array_contains(args, "-h", arr_find_str) || array_contains(args, "--help", arr_find_str) || (!run_code && !path_out) || (path_out && strlen(path_out) == 0))) {
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
            sprintf(b->sbuf, "Unsupported target: '%s'\nOptions: linux-x64, linux-arm64, macos-x64, macos-arm64, win-x64, win-arm64", target);
            build_error(b, b->sbuf);
        }
    }

    if (target_os == os_other || target_arch == arch_other) {
        sprintf(b->sbuf, "Unknown build target");
        build_error(b, b->sbuf);
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
    b->pkc_main = NULL;
    b->nsc_main = NULL;

    // Macro definitions
    MacroScope *mc = init_macro_scope(alc);
    map_set(mc->identifiers, "OS", os);
    map_set(mc->identifiers, "ARCH", arch);
    b->mc = mc;

    char *defs = map_get(options, "--def");
    if (defs) {
        build_macro_defs(b, defs);
    }

    // Filter out files
    Array *files = array_make(alc, argc);
    int argc_ = args->length;
    bool is_dir = false;
    for (int i = 2; i < argc_; i++) {
        char *arg = array_get_index(args, i);
        if (arg[0] == '-') {
            continue;
        }

        char *full = al(alc, KI_PATH_MAX);
        bool success = get_fullpath(arg, full);

        if (!success || !file_exists(full)) {
            sprintf(b->sbuf, "CLI argument, file/directory not found: '%s'", arg);
            build_error(b, b->sbuf);
        }

        if (dir_exists(full)) {
            Array *subs = get_subfiles(alc, full, false, true);
            for (int o = 0; o < subs->length; o++) {
                char *path = array_get_index(subs, o);
                if (ends_with(path, ".ki")) {
                    array_push(files, full);
                }
            }
            continue;
        }

        if (!ends_with(arg, ".ki")) {
            sprintf(b->sbuf, "CLI argument, filename must end with .ki : '%s'", arg);
            build_error(b, b->sbuf);
        }

        array_push(files, full);
    }

    if (files->length == 0) {
        sprintf(b->sbuf, "Nothing to compile, add some files or directories to your build command");
        build_error(b, b->sbuf);
    }

    char *first_file = array_get_index(files, 0);
    char first_dir[KI_PATH_MAX];
    get_dir_from_path(first_file, first_dir);
    char *cfg_dir = loader_find_config_dir(b, first_dir);
    char *main_dir = first_dir;

    // Cache dir
    char *cache_buf = al(alc, 1000);
    char *cache_hash = al(alc, 64);
    char *cache_dir = al(alc, KI_PATH_MAX);
    strcpy(cache_buf, main_dir);
    strcat(cache_buf, "||");
    strcat(cache_buf, os);
    strcat(cache_buf, arch);
    strcat(cache_buf, optimize ? "1" : "0");
    strcat(cache_buf, debug ? "1" : "0");
    strcat(cache_buf, test ? "1" : "0");
    simple_hash(cache_buf, cache_hash);
    strcpy(cache_dir, get_storage_path());
    strcat(cache_dir, "/cache/");

    if (!file_exists(cache_dir)) {
        makedir(cache_dir, 0700);
    }

    strcat(cache_dir, cache_hash);

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

    b->type = build_t_exe;
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
    b->alc_ast = alc_make();
    b->cache_dir = cache_dir;
    //
    b->event_count = 0;
    b->events_done = 0;
    b->verbose = verbose;
    //
    b->packages = array_make(alc, 100);
    b->packages_by_dir = map_make(alc);
    b->namespaces_by_dir = map_make(alc);
    b->all_ki_files = array_make(alc, 1000);
    b->link_libs = map_make(alc);
    b->link_dirs = array_make(alc, 40);
    b->fcs_by_path = map_make(alc);
    b->all_fcs = array_make(alc, 80);
    b->main_func = NULL;
    b->str_buf = str_make(alc, 5000);
    b->str_buf_io = str_make(alc_io, 10000);
    b->tests = array_make(alc, 20);
    //
    b->read_ki_file = chain_make(alc);
    b->write_ir = chain_make(alc);
    b->stage_1 = chain_make(alc);
    b->stage_2 = chain_make(alc);
    b->stage_2_1 = chain_make(alc);
    b->stage_3 = chain_make(alc);
    b->stage_4 = chain_make(alc);
    b->stage_5 = chain_make(alc);
    b->stage_6 = chain_make(alc);
    //
    b->ir_ready = false;
    b->optimize = optimize;
    b->test = test;
    b->debug = debug;
    b->clear_cache = clear_cache;
    b->run_code = run_code;
    b->LOC = 0;
    b->link_static = link_static;
    //
    b->type_void = type_gen_void(alc);

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

    // Init main:main
    Config *main_cfg = NULL;
    if (cfg_dir) {
        main_dir = cfg_dir;
        main_cfg = cfg_load(alc, b->str_buf, cfg_dir);
    }
    b->pkc_main = pkc_init(alc, b, "main", main_dir, main_cfg);
    b->nsc_main = nsc_init(alc, b, b->pkc_main, "main");
    b->pkc_main->main_nsc = b->nsc_main;

    //
    char *pkg_dir = al(alc, KI_PATH_MAX);
    strcpy(pkg_dir, main_dir);
    strcat(pkg_dir, "packages/");
    b->pkg_dir = pkg_dir;

    char *ki_dir = al(alc, KI_PATH_MAX);
    strcpy(ki_dir, get_binary_dir());
    strcat(ki_dir, "/lib/");
    Pkc *pkc_ki = loader_get_pkc_for_dir(b, ki_dir);
    b->pkc_ki = pkc_ki;

    // Load core types & functions
    loader_load_nsc(pkc_ki, "type");
    loader_load_nsc(pkc_ki, "io");
    loader_load_nsc(pkc_ki, "mem");
    loader_load_nsc(pkc_ki, "os");

    // Compile ki lib
    compile_loop(b, 1); // Scan identifiers

    // Compile CLI files
    build_add_files(b, files);
    compile_loop(b, 1); // Scan identifiers

    if (array_contains(args, "--watch", arr_find_str)) {
        build_watch(b, argc, argv);
        exit(0);
    }

    compile_loop(b, 6); // Complete all other stages

    if (b->lsp) {
        build_end(b, 0);
    }

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

    // Linker stage
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

    // Flush all output
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
        char cmd[KI_PATH_MAX];
        strcpy(cmd, "\"");
        strcat(cmd, b->path_out);
        strcat(cmd, "\"");
        int code = system(cmd);
        code = code == 0 ? 0 : 1;
        exit(code);
    }

    //
    build_end(b, 0);
}

void build_clean_up(Build *b) {
    //
    for (int i = 0; i < b->all_fcs->length; i++) {
        Fc *fc = array_get_index(b->all_fcs, i);
        if (fc->cache) {
            cJSON_Delete(fc->cache);
        }
    }
    for (int i = 0; i < b->packages->length; i++) {
        Pkc *pkc = array_get_index(b->packages, i);
        cJSON *cfg = pkc->config ? pkc->config->json : NULL;
        if (cfg) {
            cJSON_Delete(cfg);
        }
    }
    alc_delete(b->alc_ast);
    alc_delete(b->alc);
}

void build_end(Build *b, int exit_code) {
    //
    bool is_lsp = b->lsp ? true : false;
    build_clean_up(b);
    if (is_lsp)
        lsp_exit_thread();
    else
        exit(exit_code);
}

void build_error(Build *b, char *msg) {
    //
    if (!b->lsp) {
        printf("Build error: %s\n", msg);
    }
    build_end(b, 1);
}

void build_add_files(Build *b, Array *files) {
    //
    int filec = files->length;
    for (int i = 0; i < filec; i++) {
        char *path = array_get_index(files, i);
        char dir[KI_PATH_MAX];
        get_dir_from_path(path, dir);
        Nsc *nsc = loader_get_nsc_for_dir(b, dir);
        if (strcmp(nsc->name, "main") == 0) {
            fc_init(b, path, nsc, false);
        }
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
    return os_other;
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
    return arch_other;
}

void cmd_build_help(bool run_code) {
    //
    if (run_code)
        printf("\n# ki run {ki-files}\n");
    else
        printf("\n# ki build {ki-files} -o {outpath}\n");
    printf("\n");

    printf(" --clean -c          clear cache\n");
    printf(" --debug -d          generate debug info\n");
    printf(" --optimize -O       apply code optimizations\n");
    printf(" --run -r            run code after compiling\n");
    printf(" --test              generate a 'main' that runs all tests\n");
    printf(" --watch             watch files & rebuild when code changes\n");
    printf("\n");

    printf(" --def               define comptime variables\n");
    printf("                     format: VAR1=VAL,VAR2=VAL\n");
    printf(" --target            compile for a specific os/arch\n");
    printf("                     linux-x64, macos-x64, win-x64\n");
    printf(" -v -vv -vvv         show compile info\n");
    printf("\n");

    exit(1);
}

Class *ki_get_class(Build *b, char *ns, char *name) {
    //
    Pkc *pkc = b->pkc_ki;
    Nsc *nsc = pkc_get_nsc(pkc, ns);

    Idf *idf = map_get(nsc->scope->identifiers, name);
    if (!idf || idf->type != idf_class) {
        sprintf(b->sbuf, "Class not found in ki-lib '%s:%s'", ns, name);
        build_error(b, b->sbuf);
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
        build_error(b, b->sbuf);
    }

    return idf->item;
}

void build_macro_defs(Build *b, char *defs) {
    //
    MacroScope *mc = b->mc;
    Allocator *alc = b->alc;

    int len = strlen(defs);
    int i = 0;
    bool read_key = true;
    int part_i = 0;
    char part[256];
    char key[256];

    while (i < len) {
        char ch = defs[i];
        i++;
        if (ch == '=' && read_key) {
            part[part_i] = '\0';
            part_i = 0;
            if (!is_valid_varname_all(part)) {
                printf("Invalid comptime variable name: '%s'", part);
                exit(1);
            }
            strcpy(key, part);
            if (map_get(mc->identifiers, key)) {
                printf("Comptime variable name already used: '%s'", key);
                exit(1);
            }
            read_key = false;
            continue;
        }
        if (ch == ',' && !read_key) {
            part[part_i] = '\0';
            part_i = 0;

            map_set(mc->identifiers, dups(alc, key), dups(alc, part));

            read_key = true;
            continue;
        }
        part[part_i] = ch;
        part_i++;
    }
    if (!read_key) {
        part[part_i] = '\0';
        map_set(mc->identifiers, dups(alc, key), dups(alc, part));
    }
}

void build_watch(Build *b, int argc, char *argv[]) {
    //
    Str *cmd_str = str_make(b->alc, 1000);
    for (int i = 0; i < argc; i++) {
        char *arg = argv[i];
        if (strcmp(arg, "--watch") == 0)
            continue;
        str_append_chars(cmd_str, arg);
        str_append_chars(cmd_str, " ");
    }
    char *cmd = str_to_chars(b->alc, cmd_str);

    Array *fcs = b->all_fcs;
    while (true) {
        bool run = false;
        for (int i = 0; i < fcs->length; i++) {
            Fc *fc = array_get_index(fcs, i);
#ifdef _WIN32
            void *handle = fc->win_file_handle;
            if (!handle) {
                handle = CreateFile(fc->path_ki,           // file to open
                                    GENERIC_READ,          // open for reading
                                    FILE_SHARE_READ,       // share for reading
                                    NULL,                  // default security
                                    OPEN_EXISTING,         // existing file only
                                    FILE_ATTRIBUTE_NORMAL, // normal file
                                    NULL                   // no attribute template
                );
                fc->win_file_handle = handle;
            }
            FILETIME ftCreate, ftAccess, ftWrite;
            GetFileTime((HANDLE)handle, &ftCreate, &ftAccess, &ftWrite);
            long int nsec = ftWrite.dwLowDateTime + ftWrite.dwHighDateTime;
#else
            struct stat attr;
            stat(fc->path_ki, &attr);
#ifdef linux
            long int nsec = attr.st_mtim.tv_nsec;
#else
            long int nsec = attr.st_mtimespec.tv_nsec;
#endif
#endif
            if (fc->mod_time != nsec) {
                run = true;
                fc->mod_time = nsec;
            }
        }

        if (run) {
            printf("# Build...\n");
            system(cmd);
        } else {
            sleep_ms(500);
        }
    }
}
