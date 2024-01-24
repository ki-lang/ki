
#include "../all.h"

const int block_size = 1000000;
const int block_private_size = 100000;

Allocator *alc_make() {
    //
    Allocator *alc = malloc(sizeof(Allocator));
    AllocatorBlock *block = alc_block_make(block_size);
    alc->first_block = block;
    alc->last_block = block;
    alc->first_block_private = NULL;
    alc->last_block_private = NULL;

    return alc;
}
void alc_wipe(Allocator *alc) {
    //
    AllocatorBlock *block = alc->first_block;
    // Blocks
    while (block) {
        AllocatorBlock *next = block->next;
        block->current_adr = block->start_adr;
        block->space_left = block->size;
        block = next;
    }
    alc->last_block = alc->first_block;
    // Private blocks
    block = alc->first_block_private;
    while (block) {
        AllocatorBlock *next = block->next;
        free_block(block);
        block = next;
    }
    alc->first_block_private = NULL;
    alc->last_block_private = NULL;
}
AllocatorBlock *alc_block_make(size_t size) {
    //
    AllocatorBlock *block = malloc(sizeof(AllocatorBlock));
    block->next = NULL;
    block->size = size;
    block->space_left = size;

    void *adr = malloc(size);
    block->start_adr = adr;
    block->current_adr = adr;

    return block;
}
void *al(Allocator *alc, size_t size) {
    //
    if (size > block_private_size) {
        return al_private(alc, size)->start_adr;
    }

    AllocatorBlock *block = alc->last_block;

    size_t offset = size > 8 ? 8 : size;
    if ((size + offset) > block->space_left) {
        if(block->next) {
            block = block->next;
        } else {
            AllocatorBlock *block_new = alc_block_make(block_size);
            block->next = block_new;
            block = block_new;
        }
        alc->last_block = block;
    }

    intptr_t adr = (intptr_t)block->current_adr;
    size_t skip = (size_t)adr % offset;
    block->current_adr = (void*)(adr + size + skip);
    block->space_left -= size + skip;

    return (void*)(adr + skip);
}
AllocatorBlock *al_private(Allocator *alc, size_t size) {
    //
    AllocatorBlock *block = alc_block_make(size);
    block->space_left = 0;
    //
    AllocatorBlock *last = alc->last_block_private;
    if(last){
        last->next = block;
    } else {
        alc->first_block_private = block;
    }
    alc->last_block_private = block;

    return block;
}
void free_block(AllocatorBlock *block) {
    //
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
        AllocatorBlock *next = block->next;
        free_block(block);
        block = next;
    }
    block = alc->first_block_private;
    while (block) {
        AllocatorBlock *next = block->next;
        free_block(block);
        block = next;
    }
    free(alc);
}
