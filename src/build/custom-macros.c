
#include "../all.h"

void build_and_load_macros(Build *b) {
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

    bool is_linux = b->host_os == os_linux;
    bool is_macos = b->host_os == os_macos;
    bool is_win = b->host_os == os_win;

    Array *parts = array_make(b->alc, 10);

    Str *buf = b->str_buf;
    for (int i = 0; i < b->packages->length; i++) {
        Pkc *pkc = array_get_index(b->packages, i);
        if (pkc->macro_file) {
            char path_hash[64];
            char content_hash[64];
            char c_path[KI_PATH_MAX];
            char lib_path[KI_PATH_MAX];
            char header_path[KI_PATH_MAX];

            char *m_path = pkc->macro_file;
            simple_hash(m_path, path_hash);

            strcpy(c_path, b->cache_dir);
            strcat(c_path, "/macro-hash-");
            strcat(c_path, path_hash);
            strcat(c_path, ".txt");

            strcpy(lib_path, b->cache_dir);
            strcat(lib_path, "/macro-");
            strcat(lib_path, path_hash);
            if (is_win) {
                strcat(lib_path, ".dll");
            } else if (is_linux) {
                strcat(lib_path, ".so");
            } else if (is_macos) {
                strcat(lib_path, ".dylib");
            }

            strcpy(header_path, b->cache_dir);
            strcat(header_path, "/macro-");
            strcat(header_path, path_hash);
            strcat(header_path, ".txt");

            str_clear(buf);
            file_get_contents(buf, pkc->macro_file);

            char *content = str_to_chars(b->alc, buf);
            simple_hash(content, content_hash);

            bool compile = true;
            if (file_exists(c_path)) {
                str_clear(buf);
                file_get_contents(buf, c_path);
                char *old_hash = str_to_chars(b->alc, buf);
                if (strcmp(content_hash, old_hash) == 0) {
                    compile = false;
                }
            }
            // if (compile || !file_exists(header_path)) {
            if (compile || b->clear_cache) {
                // Compile macro shared lib
                int argc = 8;
                if (b->verbose > 1)
                    argc++;
                char *argv[argc];

                argv[0] = "./ki-macro";
                argv[1] = "build";
                argv[2] = "-o";
                argv[3] = lib_path;
                argv[4] = "--lib";
                argv[5] = "--build-macro-file";
                argv[6] = header_path;
                argv[7] = pkc->macro_file;

                int pos = 8;
                if (b->verbose > 2)
                    argv[pos++] = "-vv";
                else if (b->verbose > 1)
                    argv[pos++] = "-v";

                if (b->verbose > 1) {
                    printf("> Build macro: %s\n", pkc->macro_file);
                    printf("-----------------------------\n");
                }
                cmd_build(argc, argv);
                if (b->verbose > 1) {
                    printf("-----------------------------\n");
                }

                write_file(c_path, content_hash, false);
            }

            //////////////////
            // Load macro
            //////////////////

            if (b->verbose > 1) {
                printf("> Load macro: %s\n", lib_path);
            }
            void *lib = dlopen(lib_path, RTLD_NOW);

            str_clear(buf);
            file_get_contents(buf, header_path);

            int i = 0;
            char *hcontent = buf->data;
            char part[128];
            int pi = 0;
            while (i < buf->length) {
                char ch = hcontent[i];
                i++;
                if (ch == ',' || ch == '\n') {
                    part[pi] = '\0';
                    pi = 0;
                    array_push(parts, dups(b->alc, part));
                    if (ch == ',')
                        continue;
                }
                if (ch == '\n') {
                    if (parts->length != 3) {
                        break;
                    }
                    char *pattern = array_get_index(parts, 0);
                    char *alias = array_get_index(parts, 1);
                    char *func_name = array_get_index(parts, 2);
                    //

                    void *func = dlsym(lib, func_name);

                    MacroPattern *pat = al(b->alc, sizeof(MacroPattern));
                    pat->func = func;
                    pat->pattern = pattern;

                    Idf *idf = idf_init_item(b->alc, idf_macro, pat);
                    map_set(b->macros, alias, idf);

                    //
                    parts->length = 0;
                    continue;
                }
                part[pi] = ch;
                pi++;
            }

            if (b->verbose > 1) {
                printf("> Loaded macro: %s\n", lib_path);
            }
        }
    }

#ifdef WIN32
    QueryPerformanceCounter(&end);
    double time_macro = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
#else
    gettimeofday(&end, NULL);
    double time_macro = (double)(end.tv_usec - begin.tv_usec) / 1000000 + (double)(end.tv_sec - begin.tv_sec);
#endif

    if (b->verbose > 0) {
        printf("⌚ Compile macros: %.3fs\n", time_macro);
    }
}
