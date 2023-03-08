
#include "../all.h"

Allocator *alc_make() {
    //
    Allocator *alc = malloc(sizeof(Allocator));
    AllocatorBlock *block = alc_block_make(NULL, NULL, 10000);
    alc->first_block = block;
    alc->current_block = block;
    alc->last_block = block;

    return alc;
}
AllocatorBlock *alc_block_make(AllocatorBlock *prev, AllocatorBlock *next, size_t size) {
    //
    AllocatorBlock *block = malloc(sizeof(AllocatorBlock));
    block->prev_block = prev;
    block->next_block = next;
    block->size = size;
    block->space_left = size;

    void *adr = malloc(size);
    block->start_adr = adr;
    block->current_adr = adr;

    return block;
}
void *alc(Allocator *alc, size_t size) {
    //
    if (size % 8 > 0) {
        size += 8 - size % 8;
    }
    AllocatorBlock *block = alc->current_block;
    if (size <= block->space_left) {
        void *adr = block->current_adr;
        adr += size;
        block->current_adr = adr;
        block->space_left -= size;
        return adr;
    }
    AllocatorBlock *new_block = alc_block_make(block, NULL, 10000);
    block->next_block = new_block;
    void *adr = new_block->current_adr;
    adr += size;
    new_block->current_adr = adr;
    new_block->space_left -= size;
    return adr;
}