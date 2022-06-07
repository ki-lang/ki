
#include "../all.h"
#include "../libs/md5.h"

FileCompiler *init_fc() {
    FileCompiler *fc = malloc(sizeof(FileCompiler));
    fc->nsc = NULL;
    //
    fc->hash = NULL;
    fc->ki_filepath = NULL;
    fc->c_filepath = NULL;
    fc->h_filepath = NULL;
    fc->o_filepath = NULL;
    fc->is_header = false;
    fc->was_modified = false;
    fc->should_recompile = false;
    //
    fc->cache = NULL;
    //
    fc->content = NULL;
    fc->content_len = 0;
    //
    fc->i = 0;
    fc->col = 0;
    fc->line = 0;
    //
    fc->macro_results = array_make(4);
    //
    fc->c_code = str_make("");
    fc->c_code_after = str_make("");
    fc->h_code_start = str_make("");
    fc->h_code = str_make("");
    fc->tkn_buffer = NULL;
    fc->before_tkn_buffer = NULL;
    fc->value_buffer = str_make("");
    fc->indent = 0;
    fc->current_scope = NULL;
    fc->var_bufc = 0;
    fc->var_buf = malloc(12);
    //
    fc->sprintf = malloc(100);
    fc->sprintf2 = malloc(100);
    //
    fc->scope = init_scope();
    fc->uses = map_make();
    //
    fc->create_o_file = false;
    fc->classes = array_make(2);
    fc->functions = array_make(8);
    fc->enums = array_make(4);
    fc->strings = array_make(8);
    fc->globals = array_make(4);
    //
    fc->include_headers_from = array_make(10);
    return fc;
}

void free_fc(FileCompiler *fc) {
    //
    free(fc);
}

FileCompiler *fc_new_file(PkgCompiler *pkc, char *path, bool is_cmd_arg_file) {
    //
    bool is_header = ends_with(path, ".kh");
    char *ki_filepath = NULL;
    if (is_header) {
        ki_filepath = path;
    } else {
        ki_filepath = get_fullpath(path);
    }

    if (!ki_filepath) {
        printf("File not found: '%s'\n", path);
        exit(1);
    }

    if (is_cmd_arg_file) {
        array_push(cmd_arg_files, ki_filepath);
    }

    FileCompiler *fc = map_get(pkc->file_compilers, ki_filepath);
    if (fc != NULL) {
        return fc;
    }

    char *cache_dir = get_cache_dir();
    char *c_filepath = malloc(KI_PATH_MAX);
    char *h_filepath = malloc(KI_PATH_MAX);
    char *o_filepath = malloc(KI_PATH_MAX);
    char *cache_filepath = malloc(KI_PATH_MAX);
    char *x_filepath = malloc(KI_PATH_MAX);

    char *fn_ext = get_path_basename(ki_filepath);
    char *fn = strip_ext(fn_ext);
    free(fn_ext);

    char *hash = malloc(33);
    strcpy(hash, "");
    md5(ki_filepath, hash);

    strcpy(c_filepath, cache_dir);
    strcat(c_filepath, "/");
    strcat(c_filepath, fn);
    strcat(c_filepath, "_");
    strcat(c_filepath, hash);

    strcpy(h_filepath, c_filepath);
    strcpy(o_filepath, c_filepath);
    strcpy(cache_filepath, c_filepath);
    strcpy(x_filepath, c_filepath);

    strcat(c_filepath, ".c");
    strcat(h_filepath, ".h");
    strcat(o_filepath, ".o");
    strcat(cache_filepath, ".json");

    free(fn);

    fc = init_fc();
    fc->nsc = pkc_create_namespace(pkc, "main");
    fc->scope->parent = fc->nsc->scope;
    //
    fc->hash = hash;
    fc->ki_filepath = ki_filepath;
    fc->c_filepath = c_filepath;
    fc->h_filepath = h_filepath;
    fc->o_filepath = o_filepath;
    fc->cache_filepath = cache_filepath;
    fc->x_filepath = x_filepath;
    fc->is_header = is_header;
    //
    Str *content = file_get_contents(ki_filepath);
    fc->content = str_to_chars(content);
    fc->content_len = content->length;
    free(content);

    map_set(g_fc_by_ki_filepath, fc->ki_filepath, fc);
    map_set(pkc->file_compilers, fc->ki_filepath, fc);

    // Cache
    fc_load_cache(fc);

    // Check modified time
    struct stat attr;
    stat(fc->ki_filepath, &attr);
    int modtime = attr.st_mtime;
    if (modtime != fc->cache->modified_time) {
        fc->cache->modified_time = modtime;
        fc->was_modified = true;
        fc->should_recompile = true;
        fc->cache->depends_on = map_make();
    }
    if (g_nocache) {
        fc->should_recompile = true;
        fc->cache->depends_on = map_make();
    }

    // printf("Compile file: %s\n", fc->ki_filepath);

    fc_scan_types(fc);

    // printf("Compiled: %s\n", fc->ki_filepath);

    return fc;
}

void fc_include_headers_from(FileCompiler *fc, FileCompiler *from) {
    //
    if (!array_contains(fc->include_headers_from, from, "address")) {
        array_push(fc->include_headers_from, from);
    }
}

LocalVar *fc_localvar(FileCompiler *fc, char *name, Type *type) {
    LocalVar *lv = malloc(sizeof(LocalVar));
    fc->var_bufc++;
    sprintf(fc->var_buf, "_KI_LVAR_%d_%s", fc->var_bufc, name);
    lv->name = name;
    lv->gen_name = strdup(fc->var_buf);
    lv->type = type;
    return lv;
}
