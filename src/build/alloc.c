
#include "../all.h"

Allocator *alc_make() {
    //
    Allocator *alc = malloc(sizeof(Allocator));
    AllocatorBlock *block = alc_block_make(NULL, NULL, 100000);
    alc->first_block = block;
    alc->current_block = block;
    alc->last_block = block;

    return alc;
}
void alc_wipe(Allocator *alc) {
    //
    AllocatorBlock *block = alc->first_block;
    while (block) {
        AllocatorBlock *next = block->next_block;
        if (block->private) {
            free_block(block);
        } else {
            block->current_adr = block->start_adr;
            block->space_left = block->size;
        }
        block = next;
    }
}
AllocatorBlock *alc_block_make(AllocatorBlock *prev, AllocatorBlock *next, size_t size) {
    //
    AllocatorBlock *block = malloc(sizeof(AllocatorBlock));
    block->prev_block = prev;
    block->next_block = next;
    block->size = size;
    block->space_left = size;
    block->private = false;

    void *adr = malloc(size);
    block->start_adr = adr;
    block->current_adr = adr;

    return block;
}
void *al(Allocator *alc, size_t size) {
    //
    if (size > 500) {
        return al_private(alc, size)->start_adr;
    }
    if (size % 8 > 0) {
        size += 8 - (size % 8);
    }
    AllocatorBlock *block = alc->current_block;
    if (size < block->space_left) {
        void *adr = block->current_adr;
        block->current_adr += size;
        block->space_left -= size;
        // printf("l:%ld\n", block->space_left);
        return adr;
    }
    size_t new_size = 100000;
    if (size > new_size) {
        new_size = size;
    }
    AllocatorBlock *new_block = alc_block_make(block, NULL, new_size);
    alc->last_block->next_block = new_block;
    alc->last_block = new_block;
    alc->current_block = new_block;
    void *adr = new_block->current_adr;
    new_block->current_adr = adr + size;
    new_block->space_left -= size;
    return adr;
}
AllocatorBlock *al_private(Allocator *alc, size_t size) {
    //
    AllocatorBlock *last = alc->last_block;
    AllocatorBlock *block = alc_block_make(last, NULL, size);
    block->private = true;
    block->space_left = 0;
    last->next_block = block;
    return block;
}
void free_block(AllocatorBlock *block) {
    //
    AllocatorBlock *prev = block->prev_block;
    AllocatorBlock *next = block->next_block;
    if (prev) {
        prev->next_block = next;
    }
    if (next) {
        next->prev_block = prev;
    }
    free(block->start_adr);
    free(block);
}

char *dups(Allocator *alc, char *str) {
    //
    int len = strlen(str) + 1;
    void *new = al(alc, len);
    memcpy(new, str, len);
    return new;
}

void alc_delete(Allocator *alc) {
    AllocatorBlock *block = alc->first_block;
    while (block) {
        AllocatorBlock *next = block->next_block;
        free_block(block);
        block = next;
    }
    free(alc);
}
