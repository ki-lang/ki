
#include "../all.h"

Class *class_init(Allocator *alc) {
    //
    Class *class = al(alc, sizeof(class));
    class->type = ct_struct;
    class->is_rc = true;
    class->is_signed = false;
    class->packed = true;
    class->is_generic_base = false;
    class->size = 0;

    return class;
}
