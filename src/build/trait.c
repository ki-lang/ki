
#include "../all.h"

Trait *init_trait() {
    Trait *trait = malloc(sizeof(Trait));
    trait->cname = NULL;
    trait->fc = NULL;
    trait->body_i = 0;
    return trait;
}

void free_trait(Trait *trait) {
    //
    free(trait);
}