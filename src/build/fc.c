
#include "../all.h"

Fc *fc_init(Build *b, char *path_ki, Nsc *nsc) {
    //
    if (!path_ki || !file_exists(path_ki)) {
        sprintf(b->sbuf, "File not found: %s", path_ki);
        die(b->sbuf);
    }

    bool is_header = ends_with(path_ki, ".kh");

    Allocator *alc = b->alc;

    char *fn = get_path_basename(alc, path_ki);
    fn = strip_ext(alc, fn);

    char *path_ir = al(alc, KI_PATH_MAX);
    sprintf(path_ir, "%s/%s_%s_%s.ir", b->cache_dir, nsc->name, fn, nsc->pkc->hash);

    Fc *fc = al(alc, sizeof(Fc));
    fc->b = b;
    fc->path_ki = path_ki;
    fc->path_ir = path_ir;
    fc->ir = NULL;
    fc->ir_hash = "";
    fc->nsc = nsc;
    fc->alc = alc;
    fc->alc_ast = b->alc_ast;
    fc->deps = array_make(alc, 20);
    fc->token = b->token;
    fc->sbuf = b->sbuf;
    fc->chunk = chunk_init(alc);
    fc->chunk_prev = chunk_init(alc);
    fc->scope = scope_init(alc, sct_fc, nsc->scope, false);
    fc->is_header = is_header;
    fc->funcs = array_make(alc, 20);
    fc->classes = array_make(alc, 4);
    fc->id_buf = id_init(alc);
    fc->class_size_checks = array_make(alc, 4);
    fc->type_size_checks = array_make(alc, 20);

    Str *buf = str_make(alc, 500);
    file_get_contents(buf, fc->path_ki);
    char *content = str_to_chars(alc, buf);
    fc->chunk->content = content;
    fc->chunk->length = strlen(content);
    chain_add(b->stage_1, fc);
    // chain_add(b->read_ki_file, fc);
    // b->event_count++;

    array_push(nsc->fcs, fc);

    return fc;
}

Chain *chain_make(Allocator *alc) {
    Chain *chain = al(alc, sizeof(Chain));
    chain->alc = alc;
    chain->first = NULL;
    chain->last = NULL;
    chain->current = NULL;
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
    printf("File: %s\n", fc->path_ki);
    printf("Line: %d\n", line);
    printf("Col: %d\n", col);
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
