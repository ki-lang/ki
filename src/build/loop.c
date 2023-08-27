
#include "../all.h"

void compile_loop(Build *b, int max_stage) {

    Array *stages = array_make(b->alc, 20);
    array_push(stages, b->stage_1);
    array_push(stages, b->stage_2_1);
    array_push(stages, b->stage_2_2);
    array_push(stages, b->stage_2_3);
    array_push(stages, b->stage_2_4);
    array_push(stages, b->stage_2_5);
    array_push(stages, b->stage_2_6);
    array_push(stages, b->stage_3);
    array_push(stages, b->stage_4_1);

    const int stage_count = max_stage > 0 ? 1 : stages->length;

    for (int i = 0; i < stage_count; i++) {
        bool did_work = false;
        Chain *stage = array_get_index(stages, i);
        Fc *fc = chain_get(stage);
        while (fc) {
            stage->func(fc);
            fc->stage_completed = i + 1;
            fc = chain_get(stage);
            did_work = true;
        }
        if (did_work) {
            i = -1;
            continue;
        }
    }
}

Chain *chain_make(Allocator *alc, void (*func)(Fc *)) {
    Chain *chain = al(alc, sizeof(Chain));
    chain->alc = alc;
    chain->first = NULL;
    chain->last = NULL;
    chain->current = NULL;
    chain->func = func;
    return chain;
}
Fc *chain_get(Chain *chain) {
    //
    if (chain->current == NULL) {
        chain->current = chain->first;
        ChainItem *cur = chain->current;
        if (cur) {
            return cur->item;
        }
        return NULL;
    }
    ChainItem *next = chain->current->next;
    if (next) {
        chain->current = next;
        return next->item;
    }
    return NULL;
}

void chain_add(Chain *chain, Fc *item) {
    //
    ChainItem *ci = al(chain->alc, sizeof(ChainItem));
    ci->item = item;
    ci->next = NULL;
    if (chain->first == NULL) {
        chain->first = ci;
        chain->last = ci;
        return;
    }
    ChainItem *last = chain->last;
    last->next = ci;
    chain->last = ci;
}
