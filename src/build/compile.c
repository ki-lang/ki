
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

    result = run_cmd(cmd);

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
                fc_compile_o_file(fc);
            }
        }
    }

    fc_compile_local_c_files();

    //
    wait_cmd();
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

    // strcat(lib_dir, lib_dir_suffix);

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

    // Object files
    for (int i = 0; i < o_files->length; i++) {
        strcat(cmd, " ");
        strcat(cmd, array_get_index(o_files, i));
    }

    // Output file
    strcat(cmd, " -o ");
    strcat(cmd, g_output_name);

    for (int i = 0; i < g_link_dirs->length; i++) {
        char *dir = array_get_index(g_link_dirs, i);
        strcat(cmd, " -L ");
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

    strcat(cmd, " -lz -lpthread -pthread");
#ifdef _WIN32
    strcat(cmd, " -lws2_32");
#else
    strcat(cmd, " -ldl");
#endif

    // Compile
    if (g_verbose) {
        printf("%s\n", cmd);
    }
    int result = run_cmd(cmd);
    if (result == -1) {
        printf("Compile failed\n");
        exit(1);
    }
    wait_cmd();

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

    int result = run_cmd(cmd);
    if (result == -1) {
        printf("Compile .o failed\n");
        exit(1);
    }

    free(cmd);
}

////////

char *get_compiler_path() {
    char *cmd = malloc(1000);
    strcpy(cmd, "gcc");
    return cmd;
    strcpy(cmd, get_binary_dir());
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    strcat(cmd, "/misc/tcc/win/tcc.exe");
#else
    strcat(cmd, "/misc/tcc/linux/tcc");
#endif
    return cmd;
}

int run_cmd(char *cmd) {
    int result = 0;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    result = system(cmd);
#else
    child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
    } else if (child_pid == 0) {
        result = execlp("/bin/sh", "/bin/sh", "-c", cmd, NULL);
    }
#endif
    return result;
}

void wait_cmd() {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#else
    while ((wpid = wait(&status)) > 0)
        ;
#endif
}