
#include "../all.h"

void pkc_init(Pkc *pkc, Build *b) {
    //
    pkc->namespaces = b_alloc(b, sizeof(Map));
}
