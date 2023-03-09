
#include "../all.h"

Pkc *pkc_init(Allocator *alc, Build *b, char *name) {
    //
    Pkc *pkc = al(alc, sizeof(Pkc));
    pkc->b = b;
    pkc->namespaces = al(alc, sizeof(Map));
    pkc->name = name;
    return pkc;
}

Nsc *pkc_get_nsc(Pkc *pkc, char *name) {
    //
    Nsc *nsc = map_get(pkc->namespaces, name);
    if (!nsc) {
        sprintf(pkc->b->sbuf, "Namespace not found: '%s'", name);
        die(pkc->b->sbuf);
    }
    return nsc;
}
