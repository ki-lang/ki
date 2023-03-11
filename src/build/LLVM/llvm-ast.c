
#include "../../headers/LLVM.h"

char *llvm_write_ast(LB *b, Scope *scope) {
    //
    if (!scope->lvars)
        scope->lvars = map_make(b->alc);
}
