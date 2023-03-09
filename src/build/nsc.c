
#include "../all.h"

Nsc *nsc_init(Allocator *alc, Build *b, Pkc *pkc, char *name) {
    //
    Nsc *nsc = al(alc, sizeof(Nsc));
    nsc->b = b;
    nsc->pkc = pkc;
    nsc->name = name;
    nsc->scope = scope_init(alc, sct_default, NULL);
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