
#include "all.h"

void cmd_build(Array *files, Map *options) {

    if (map_get(options, "-h") || files->length == 0) {
        build_help();
        return;
    }

    LOC = 0;
    double parse_time, used_time, write_c_time, compile_time;
    parse_time = get_time();

    printf("Build...\n");

    //
    cmd_build_init_static();
    // Globals
    packages = map_make();
    headers = array_make(10);
    o_files = array_make(10);
    allocators = map_make();
    cmd_arg_files = array_make(2);
    macro_defines = map_make();
    g_fc_by_ki_filepath = map_make();

    c_identifiers = map_make();
    c_struct_identifiers = map_make();
    c_enum_identifiers = map_make();
    c_union_identifiers = map_make();
    allow_new_namespaces = true;
    build_ast_stage = false;
    uses_async = false;
    last_readonly_i = 0;
    //
    g_main_func = NULL;
    g_links = array_make(4);
    g_functions = array_make(80);
    g_classes = array_make(30);
    g_link_dirs = array_make(2);
    g_test_funcs = array_make(4);

    // -static option
    g_static = true;
    g_optimize = false;
    g_verbose = false;
    g_nocache = false;
    g_optimize = false;
    g_run = false;
    g_run_tests = false;
    g_sprintf = malloc(1000);

    g_output_name = map_get(options, "-o");

    if (map_contains(options, "--run")) {
        g_run = true;
    }
    if (map_contains(options, "--tests")) {
        g_run_tests = true;
    }
    if (map_contains(options, "--shared")) {
        g_static = false;
    }
    if (map_contains(options, "-v")) {
        g_verbose = true;
    }
    if (map_contains(options, "-vvv")) {
        g_verbose = true;
        g_verbose_all = true;
    }
    if (map_contains(options, "--clean")) {
        g_nocache = true;
    }
    if (map_contains(options, "--optimize")) {
        g_optimize = true;
    }
    if (map_contains(options, "--release")) {
        g_nocache = true;
        g_optimize = true;
    }

    if (!g_output_name) {
        char *cache_dir = get_cache_dir();
        g_output_name = malloc(KI_PATH_MAX);
        strcpy(g_output_name, cache_dir);
        strcat(g_output_name, "/build_and_run");
    }

    //
    cmd_build_init_before_build();
    //
    PkgCompiler *pkc = init_pkc();
    pkc->name = "main";
    map_set(packages, pkc->name, pkc);

    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    pkc->dir = strdup(cwd);
    pkc_check_config(pkc);

    pkc_get_namespace_or_create(pkc, "main");

    // Step 1. create fc and scan types
    // Step 1.1 scan headers
    if (g_verbose)
        printf("# SCAN TYPES\n");
    for (int i = 0; i < files->length; i++) {
        char *filepath = array_get_index(files, i);
        fc_new_file(pkc, filepath, true);
    }

    allow_new_namespaces = false;

    // Step 2. Scan values
    if (g_verbose)
        printf("# CHECK CACHE\n");
    build_cache_checks();

    // Step 3. Scan values
    if (g_verbose)
        printf("# SCAN ARGS/PROPS\n");
    fc_scan_values();

    build_ast_stage = true;

    // Step 4. Build ASTs
    if (g_verbose)
        printf("# BUILD AST\n");
    fc_build_asts();
    parse_time = get_time() - parse_time;

    //
    used_time = get_time();
    if (g_verbose)
        printf("# MARK USED CODE\n");
    mark_used_files();
    used_time = get_time() - used_time;

    // Step 5. Write c
    if (g_verbose)
        printf("# TRANSLATE AST\n");
    write_c_time = get_time();
    fc_write_c_all();
    write_c_time = get_time() - write_c_time;

    // Step 6. Compile
    if (g_verbose)
        printf("# COMPILE\n");
    compile_time = get_time();
    compile_all();
    compile_time = get_time() - compile_time;

    // Store cache
    save_cache();

    // Free variables

    //
    printf("\n");
    printf("Parse time: %f\n", parse_time);
    printf("Mark used time: %f\n", used_time);
    printf("Write-c time: %f\n", write_c_time);
    printf("Compile time: %f\n", compile_time);
    printf("Lines of code: %d\n", LOC);
    printf("\n");

    // Run
    if (g_run) {
        char *cmd = malloc(KI_PATH_MAX);
        // Set chmod
        if (!file_exists(g_output_name)) {
            die("Output file not found...");
        }
        chmod(g_output_name, 0777);
        sync();
        //
        printf("# Run: %s\n", g_output_name);
        strcpy(cmd, g_output_name);
        run_cmd(cmd);
        wait_cmd();
        if (cmd_err_code != 0) {
            printf("# Failed to run\n");
            exit(1);
        }
    }
}

void cmd_build_init_static() {
    //
    if (internal_types != NULL) {
        return;
    }

    pointer_size = 8;

    internal_types = array_make(10);
    array_push(internal_types, "ixx");
    array_push(internal_types, "uxx");
    array_push(internal_types, "i8");
    array_push(internal_types, "u8");
    array_push(internal_types, "i16");
    array_push(internal_types, "u16");
    array_push(internal_types, "i32");
    array_push(internal_types, "u32");
    array_push(internal_types, "i64");
    array_push(internal_types, "u64");
    array_push(internal_types, "ptr");
    array_push(internal_types, "bool");
    array_push(internal_types, "String");
    array_push(internal_types, "cstring");
    array_push(internal_types, "Array");
    array_push(internal_types, "Map");
    array_push(internal_types, "Value");
}

#ifdef WIN32

double get_time() {
    LARGE_INTEGER t, f;
    QueryPerformanceCounter(&t);
    QueryPerformanceFrequency(&f);
    return (double)t.QuadPart / (double)f.QuadPart;
}

#else

double get_time() {
    struct timeval t;
    struct timezone tzp;
    gettimeofday(&t, &tzp);
    return t.tv_sec + t.tv_usec * 1e-6;
}

#endif

void cmd_build_init_before_build() {

    map_set(macro_defines, "OS_WIN", "0");
    map_set(macro_defines, "OS_OSX", "0");
    map_set(macro_defines, "OS_LINUX", "0");

#if defined _WIN32
    map_set(macro_defines, "OS_WIN", "1");
#elif defined __APPLE__
    map_set(macro_defines, "OS_OSX", "1");
#else
    map_set(macro_defines, "OS_LINUX", "1");
#endif
}

void build_help() {
    //
    printf("Usage: ki build {files} -o {output name/path}\n\n");

    printf("> Example: ki build src/*.ki -o myapp\n\n");

    printf("  --run                Run program after compiling\n");
    printf("  --clean              Clean build, ignore cache\n");
    printf("  --optimize           Enable optimizations (-O3)\n");
    printf("  --release            Release build, no cache, optimized\n");
    printf("  -v -vvv              Verbose, outputs extra information\n");
    printf("  --tests              Include _test_ functions and run them at start\n");

    // Hide for now
    // printf("  --shared             Use shared libraries\n");

    printf("\n");
}