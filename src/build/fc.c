
#include "../all.h"

Fc *fc_init(Build *b, char *path_ki, bool duplicate) {
    //
    Fc *prev = map_get(b->all_fcs, path_ki);
    if (prev && !duplicate) {
        return prev;
    }

    //
    if (!file_exists(path_ki)) {
        if (b->lsp)
            lsp_exit_thread();
        sprintf(b->sbuf, "File not found: %s", path_ki);
        die(b->sbuf);
    }

    bool is_header = ends_with(path_ki, ".kh");

    Allocator *alc = b->alc;

    // Pkc / Nsc
    Pkc *pkc = loader_find_pkc_for_file(b, path_ki);
    Nsc *nsc = loader_get_nsc_for_file(pkc, path_ki);

    Fc *fc = al(alc, sizeof(Fc));
    fc->b = b;
    fc->path_ki = path_ki;
    fc->path_ir = NULL;
    fc->path_cache = NULL;
    fc->ir = NULL;
    fc->ir_hash = "";
    fc->nsc = nsc;
    fc->alc = alc;
    fc->alc_ast = b->alc_ast;
    fc->deps = array_make(alc, 20);
    fc->sub_headers = array_make(alc, 2);
    fc->token = b->token;
    fc->sbuf = b->sbuf;
    fc->chunk = chunk_init(alc, fc);
    fc->chunk_prev = chunk_init(alc, fc);
    fc->id_buf = id_init(alc);
    fc->str_buf = str_make(alc, 100);

    fc->scope = scope_init(alc, sct_fc, nsc->scope, false);
    fc->current_macro_scope = b->mc;

    fc->error_func_info = NULL;
    fc->error_class_info = NULL;

    fc->funcs = array_make(alc, 20);
    fc->classes = array_make(alc, 4);
    fc->globals = array_make(alc, 4);
    fc->aliasses = array_make(alc, 4);
    fc->class_size_checks = array_make(alc, 4);
    fc->type_size_checks = array_make(alc, 20);
    fc->extends = array_make(alc, 4);

    fc->cache = NULL;
    fc->is_header = is_header;
    fc->ir_changed = false;
    fc->win_file_handle = NULL;
    fc->mod_time = 0;
    fc->lsp_file = b->lsp && (strcmp(b->lsp->filepath, path_ki) == 0);

    fc->test_counter = 0;

    //
    fc_set_cache_paths(fc);

    Str *buf = str_make(alc, 500);
    if (file_exists(fc->path_cache)) {
        file_get_contents(buf, fc->path_cache);
        char *content = str_to_chars(alc, buf);
        fc->cache = cJSON_ParseWithLength(content, buf->length);
        cJSON *item = cJSON_GetObjectItemCaseSensitive(fc->cache, "ir_hash");
        if (item) {
            fc->ir_hash = item->valuestring;
        }
    }

    // Content
    if (prev) {
        fc->chunk = chunk_clone(alc, prev->chunk);
    } else {
        str_clear(buf);

        char *content = NULL;
        if (fc->lsp_file && b->lsp->text) {
            lsp_log("LSP Custom content\n");
            content = b->lsp->text;
        } else if (lsp_doc_content) {
            content = map_get(lsp_doc_content, fc->path_ki);
            if (content) {
                content = dups(alc, content);
            }
        }
        if (!content) {
            file_get_contents(buf, fc->path_ki);
            content = str_to_chars(alc, buf);
        }
        fc->chunk->content = content;
        fc->chunk->length = strlen(content);

        chain_add(b->stage_1, fc);
    }

    //
    array_push(nsc->fcs, fc);
    map_set(b->all_fcs, path_ki, fc);

    // printf("FC: %s\n", path_ki);

    return fc;
}

void fc_set_cache_paths(Fc *fc) {

    Build *b = fc->b;
    Allocator *alc = b->alc;
    Nsc *nsc = fc->nsc;

    char *path_ir = al(alc, KI_PATH_MAX);
    char *path_cache = al(alc, KI_PATH_MAX);
    char *fn = get_path_basename(alc, fc->path_ki);
    fn = strip_ext(alc, fn);
    sprintf(path_ir, "%s/%s_%s_%s.ir", b->cache_dir, nsc->name, fn, nsc->pkc->hash);
    sprintf(path_cache, "%s/%s_%s_%s.json", b->cache_dir, nsc->name, fn, nsc->pkc->hash);

    fc->path_ir = path_ir;
    fc->path_cache = path_cache;

    char *hash = al(alc, 64);
    simple_hash(fc->path_ki, hash);
    fc->path_hash = hash;
}

Chain *chain_make(Allocator *alc) {
    Chain *chain = al(alc, sizeof(Chain));
    chain->alc = alc;
    chain->first = NULL;
    chain->last = NULL;
    chain->current = NULL;
    return chain;
}
Fc *chain_get(Chain *chain) {
    //
    if (chain->current == NULL) {
        chain->current = chain->first;
        ChainItem *cur = chain->current;
        if (cur) {
            return cur->item;
        }
        return NULL;
    }
    ChainItem *next = chain->current->next;
    if (next) {
        chain->current = next;
        return next->item;
    }
    return NULL;
}

void chain_add(Chain *chain, Fc *item) {
    //
    ChainItem *ci = al(chain->alc, sizeof(ChainItem));
    ci->item = item;
    ci->next = NULL;
    if (chain->first == NULL) {
        chain->first = ci;
        chain->last = ci;
        return;
    }
    ChainItem *last = chain->last;
    last->next = ci;
    chain->last = ci;
}

void fc_error(Fc *fc) {
    //
    Chunk *chunk = fc->chunk;
    char *content = chunk->content;
    int length = chunk->length;

    Build *b = fc->b;
    if (b->lsp) {
        lsp_log("Error: ");
        lsp_log(fc->sbuf);
        lsp_log("\n");
        b->lsp->send_default = true;
        lsp_exit_thread();
    }

    if (is_newline(get_char(fc, 0))) {
        chunk->i--;
    }

    int line = chunk->line;
    int i = chunk->i;

    int col = 0;
    i = chunk->i;
    while (i >= 0) {
        char ch = content[i];
        if (is_newline(ch)) {
            i++;
            break;
        }
        col++;
        i--;
    }
    int start = i;

    printf("\n");
    Chunk *parent = chunk->parent;
    while (parent) {
        printf("File: %s\n", parent->fc ? parent->fc->path_ki : "?");
        printf("At: line:%d | col:%d\n", parent->line, parent->col);
        printf("-------------------------------------\n");
        parent = parent->parent;
    }
    printf("File: %s\n", chunk->fc ? chunk->fc->path_ki : "?");
    if (fc->error_class_info) {
        printf("Class: %s\n", fc->error_class_info->dname);
    }
    if (fc->error_func_info) {
        printf("Function: %s\n", fc->error_func_info->dname);
    }
    printf("At: line:%d | col:%d\n", chunk->line, chunk->col);
    printf("Error: %s\n", fc->sbuf);
    printf("\n");

    // Line 1
    int c = 40;
    while (c > 0) {
        printf("#");
        c--;
    }
    printf("\n");

    // Code
    i = chunk->i;
    while (i < length) {
        char ch = content[i];
        if (is_newline(ch)) {
            break;
        }
        i++;
    }
    int end = i;
    i = start;
    while (i < end) {
        char ch = content[i];
        if (ch == '\t') {
            ch = ' ';
        }
        printf("%c", ch);
        i++;
    }
    printf("\n");

    // Line 2
    c = col - 3;
    int after_c = 40 - c - 4;
    while (c >= 0) {
        printf("#");
        c--;
    }
    printf(" ^ ");
    while (after_c > 0) {
        printf("#");
        after_c--;
    }
    printf("\n");

    printf("\n");
    exit(1);
}

void fc_update_cache(Fc *fc) {
    //
    bool save = false;
    cJSON *cache = fc->cache;

    if (!cache) {
        cache = cJSON_CreateObject();
        fc->cache = cache;
    }

    if (fc->ir_changed) {
        save = true;

        cJSON *item = cJSON_GetObjectItemCaseSensitive(cache, "ir_hash");
        if (!item) {
            item = cJSON_CreateString(fc->ir_hash);
            cJSON_AddItemToObject(cache, "ir_hash", item);
        } else {
            cJSON_SetValuestring(item, fc->ir_hash);
        }
    }

    if (save) {
        char *content = cJSON_Print(cache);
        if (content) {
            write_file(fc->path_cache, content, false);
            free(content);
        }
    }
}