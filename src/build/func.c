
#include "../all.h"

Func *func_init(Allocator *alc) {
    //
    Func *func = al(alc, sizeof(Func));

    return func;
}
