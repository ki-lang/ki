
#include "../all.h"

Fc *fc_init(Build *b, char *path_ki, Nsc *nsc) {
    //
    if (!path_ki || !file_exists(path_ki)) {
        sprintf(die_buf, "File not found: %s", path_ki);
        die(die_buf);
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
    fc->content = NULL;
    fc->nsc = nsc;
    fc->alc = alc;
    fc->alc_ast = b->alc_ast;
    fc->deps = array_make(alc, 20);
    fc->stage = 0;

    chain_add(b->read_ki_file, fc);
    b->event_count++;

    return fc;
}

Chain *chain_make(Allocator *alc) {
    Chain *chain = al(alc, sizeof(Chain));
    chain->first = NULL;
    chain->last = NULL;
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