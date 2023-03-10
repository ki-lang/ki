
#include "../all.h"

Func *func_init(Allocator *alc) {
    //
    Func *func = al(alc, sizeof(Func));
    func->rett = NULL;
    func->args = array_make(alc, 8);
    func->args_by_name = map_make(alc);
    func->act = 0;
    func->is_static = false;

    return func;
}
