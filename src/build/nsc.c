
#include "../all.h"

Nsc *nsc_init(Allocator *alc, Build *b, Pkc *pkc, char *name) {
    //
    Nsc *nsc = al(alc, sizeof(Nsc));
    nsc->b = b;
    nsc->pkc = pkc;
    nsc->name = dups(alc, name);
    nsc->scope = scope_init(alc, sct_default, b->root_scope, false);
    nsc->fcs = array_make(alc, 40);

    //
    char *o_path = al(alc, KI_PATH_MAX);
    strcpy(o_path, b->cache_dir);
    strcat(o_path, "/nsc_");
    strcat(o_path, pkc->hash);
    strcat(o_path, "_");
    strcat(o_path, nsc->name);
    strcat(o_path, ".o");

    nsc->path_o = o_path;

    map_set(pkc->namespaces, nsc->name, nsc);

    return nsc;
}

char *nsc_gname(Nsc *nsc, char *name) {
    //
    Pkc *pkc = nsc->pkc;
    char *buf = nsc->b->sbuf;
    strcpy(buf, "");
    if (strcmp(pkc->name, "main") != 0) {
        strcat(buf, pkc->name);
        strcat(buf, "__");
    }
    if (strcmp(nsc->name, "main") != 0) {
        strcat(buf, nsc->name);
        strcat(buf, "__");
    }
    strcat(buf, name);
    return dups(nsc->b->alc, buf);
}
char *nsc_dname(Nsc *nsc, char *name) {
    //
    Pkc *pkc = nsc->pkc;
    char *buf = nsc->b->sbuf;
    strcpy(buf, "");
    if (strcmp(pkc->name, "main") != 0) {
        strcat(buf, pkc->name);
        strcat(buf, ":");
    }
    if (strcmp(nsc->name, "main") != 0) {
        strcat(buf, nsc->name);
        strcat(buf, ":");
    }
    strcat(buf, name);
    return dups(nsc->b->alc, buf);
}