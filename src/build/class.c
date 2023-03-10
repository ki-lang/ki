
#include "../all.h"

Class *class_init(Allocator *alc) {
    //
    Class *class = al(alc, sizeof(class));
    class->is_rc = true;
    class->packed = true;

    return class;
}
