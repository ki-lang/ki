
#include "../all.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#else
pid_t child_pid, wpid;
int status = 0;
#endif

char basic_args[100];
char *fc_compile_basic_args() {
    //
    strcpy(basic_args, " -g -fcommon -c");
    strcat(basic_args, g_optimize ? " -O3" : " -O0");
    return basic_args;
}

void fc_compile_local_c_files() {
    //
    int result;
    char *cache_dir = get_cache_dir();
    char *c_file = malloc(3000);
    char *o_file = malloc(3000);
    char *cmd = malloc(3000);

    //
    strcpy(c_file, cache_dir);
    strcat(c_file, "/inits.c");

    strcpy(o_file, cache_dir);
    strcat(o_file, "/inits.o");

    array_push(o_files, strdup(o_file));

    strcpy(cmd, get_compiler_path());
    strcat(cmd, fc_compile_basic_args());
    strcat(cmd, " -I ");
    strcat(cmd, get_binary_dir());
    strcat(cmd, " -o ");
    strcat(cmd, o_file);
    strcat(cmd, " ");
    strcat(cmd, c_file);

    run_cmd(cmd);

    //
    free(cmd);
    free(c_file);
    free(o_file);
}

void compile_all() {
    if (o_files->length == 0) {
        printf("Nothing to compile\n");
        return;
    }

    //
    for (int i = 0; i < packages->keys->length; i++) {
        PkgCompiler *pkc = array_get_index(packages->values, i);
        for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
            FileCompiler *fc = array_get_index(pkc->file_compilers->values, o);
            if (fc->create_o_file && fc->should_recompile) {
                if (cmd_err_code == 0) {
                    fc_compile_o_file(fc);
                }
            }
        }
    }

    if (cmd_err_code == 0) {
        fc_compile_local_c_files();
    }

    //
    wait_cmd();

    if (cmd_err_code != 0) {
        printf("Failed to compile code\n");
        exit(1);
    }

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#else
    sync();
#endif

    // char *lib_dir = malloc(KI_PATH_MAX);
    // strcpy(lib_dir, get_binary_dir());
    // strcat(lib_dir, "lib/binaries");

    char *lib_dir_suffix = malloc(KI_PATH_MAX);
    strcpy(lib_dir_suffix, "/");
#if defined _WIN32
    strcat(lib_dir_suffix, "win-x64");
#elif defined __APPLE__
    strcat(lib_dir_suffix, "osx-x64");
#else
    strcat(lib_dir_suffix, "linux-x64");
#endif
    strcat(lib_dir_suffix, g_static ? "/static" : "/shared");

    // Check if mold installed
    // run_cmd("which mold");
    // wait_cmd();
    // bool has_mold = cmd_err_code == 0;
    //

    // Compile executable
    char *cmd = malloc(50000);
    strcpy(cmd, "");

    // Set LD_RUN_PATH
    // if (!g_static) {
    //     strcat(cmd, "LD_RUN_PATH='$ORIGIN/");
    //     strcat(cmd, lib_dir);
    //     strcat(cmd, "' ");
    // }

    // gcc/clang/...
    strcat(cmd, get_compiler_path());

    // Mold
    // if (has_mold) {
    // strcat(cmd, " -fuse-ld=mold");
    // }

    // Object files
    for (int i = 0; i < o_files->length; i++) {
        strcat(cmd, " ");
        strcat(cmd, array_get_index(o_files, i));
    }

    // Output file
    strcat(cmd, " -o ");
    strcat(cmd, g_output_name);

    // Link dir
    for (int i = 0; i < g_link_dirs->length; i++) {
        char *dir = array_get_index(g_link_dirs, i);
        strcat(cmd, " -L");
        strcat(cmd, dir);
        strcat(cmd, lib_dir_suffix);
    }

    // Link libraries
    //#ifndef __APPLE__
    //    strcat(cmd, " -Wl,--disable-new-dtags");
    //#endif

    for (int i = 0; i < g_links->length; i++) {
        char *link = array_get_index(g_links, i);
        strcat(cmd, " -l");
        strcat(cmd, link);
    }

#ifdef _WIN32
    strcat(cmd, " -lws2_32");
#else
    strcat(cmd, " -ldl");
#endif
    strcat(cmd, " -lpthread -pthread");

    // Compile
    if (g_verbose) {
        printf("%s\n", cmd);
    }

    run_cmd(cmd);
    wait_cmd();

    if (cmd_err_code != 0) {
        printf("Failed to compile executable\n");
        exit(1);
    }

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#else
    sync();
#endif

    free(cmd);
}

void fc_compile_o_file(FileCompiler *fc) {
    //
    char *cmd = malloc(3000);
    strcpy(cmd, get_compiler_path());
    strcat(cmd, fc_compile_basic_args());
    strcat(cmd, " -I ");
    strcat(cmd, get_binary_dir());
    strcat(cmd, " -o ");
    strcat(cmd, fc->o_filepath);
    strcat(cmd, " ");
    strcat(cmd, fc->c_filepath);

    if (g_verbose) {
        printf("Write .o: %s\n", fc->o_filepath);
    }

    run_cmd(cmd);

    free(cmd);
}

////////

char *get_compiler_path() {
    return "gcc";
    // return "clang";
    //     strcpy(cmd, get_binary_dir());
    // #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    //     return "/misc/tcc/win/tcc.exe";
    // #endif
}

int cmd_init_c = 0;
int cmd_done_c = 0;
int cmd_err_code = 0;
void *run_cmd_x(void *cmd_v) {
    //
    char *cmd = (char *)cmd_v;
    int result = system(cmd);
    if (result != 0) {
        cmd_err_code = result;
    }
    cmd_done_c++;
}

void run_cmd(char *cmd) {
    // Run max 10 at the same time
    while (cmd_init_c - 10 > cmd_done_c)
        ;
    cmd_init_c++;
    pthread_t *pt = malloc(sizeof(pthread_t));
    pthread_create(pt, NULL, run_cmd_x, strdup(cmd));
}

void wait_cmd() {
    //
    while (cmd_init_c != cmd_done_c)
        ;
}