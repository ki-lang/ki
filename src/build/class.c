
#include "../all.h"

Class *class_init(Allocator *alc) {
    //
    Class *class = al(alc, sizeof(class));

    return class;
}
