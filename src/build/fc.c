
#include "../all.h"

Fc *fc_init(Build *b, char *path_ki, Nsc *nsc) {
    //
    if (!path_ki || !file_exists(path_ki)) {
        sprintf(b->msg, "File not found: %s", path_ki);
        die(b->msg);
    }

    Allocator *alc = b->alc;

    char *strg_dir = get_storage_path();
    char *fn = get_path_basename(alc, path_ki);
    fn = strip_ext(alc, fn);

    char *path_ir = al(alc, KI_PATH_MAX);
    sprintf(path_ir, "%s/%s.ir", strg_dir, fn);

    Fc *fc = al(alc, sizeof(Fc));
    fc->b = b;
    fc->next = NULL;
    fc->path_ki = path_ki;
    fc->path_ir = path_ir;
    fc->nsc = nsc;
    fc->alc = alc;
    fc->alc_ast = b->alc_ast;
    fc->deps = array_make(alc, 20);
    fc->stage = 0;
    fc->token = al(alc, KI_TOKEN_MAX);
    fc->chunk = chunk_init(alc);
    fc->chunk_prev = chunk_init(alc);

    chain_add(b->read_ki_file, fc);
    b->event_count++;

    return fc;
}

Chain *chain_make(Allocator *alc) {
    Chain *chain = al(alc, sizeof(Chain));
    chain->first = NULL;
    chain->last = NULL;
    chain->current = NULL;
}
Fc *chain_get(Chain *chain) {
    //
    if (chain->current == NULL) {
        chain->current = chain->first;
        return chain->current;
    }
    Fc *next = chain->current->next;
    if (next) {
        chain->current = next;
    }
    return next;
}

void chain_add(Chain *chain, Fc *item) {
    //
    if (chain->first == NULL) {
        chain->first = item;
        chain->last = item;
        return;
    }
    Fc *last = chain->last;
    last->next = item;
    chain->last = item;
}

void fc_error(Fc *fc, char *msg) {
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
    printf("Error: %s\n", msg);
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
