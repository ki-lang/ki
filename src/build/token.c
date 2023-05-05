
#include "../all.h"

Token *token_init(Allocator *alc, int type, void *item) {
    //
    Token *t = al(alc, sizeof(Token));
    t->type = type;
    t->item = item;

    return t;
}

TIf *tgen_tif(Allocator *alc, Value *cond, Scope *scope, Scope *else_scope, Scope *deref_scope) {
    //
    TIf *tif = al(alc, sizeof(TIf));
    tif->cond = cond;
    tif->scope = scope;
    tif->else_scope = else_scope;
    tif->deref_scope = deref_scope;
    return tif;
}

Token *tgen_assign(Allocator *alc, Value *left, Value *right) {
    //
    VPair *pair = al(alc, sizeof(VPair));
    pair->left = left;
    pair->right = right;
    return token_init(alc, tkn_assign, pair);
}

Token *tgen_return(Allocator *alc, Scope *fscope, Value *retv) {
    //
    return token_init(alc, tkn_return, retv);
}
Token *tgen_vscope_return(Allocator *alc, Scope *vscope, Value *retv) {
    //
    TReturnVscope *ret = al(alc, sizeof(TReturnVscope));
    ret->scope = vscope;
    ret->value = retv;
    return token_init(alc, tkn_vscope_return, ret);
}

Token *tgen_while(Allocator *alc, Value *cond, Scope *scope) {
    //
    TWhile *w = al(alc, sizeof(TWhile));
    w->cond = cond;
    w->scope = scope;
    return token_init(alc, tkn_while, w);
}

Token *tgen_exec(Allocator *alc, Scope *scope, bool enable) {
    //
    TExec *item = al(alc, sizeof(TExec));
    item->scope = scope;
    item->enable = enable;
    return token_init(alc, tkn_exec, item);
}

Token *tgen_each(Allocator *alc, Value *value, Scope *scope, Decl *decl_key, Decl *decl_value) {
    //
    TEach *item = al(alc, sizeof(TEach));
    item->value = value;
    item->scope = scope;
    item->decl_key = decl_key;
    item->decl_value = decl_value;
    return token_init(alc, tkn_each, item);
}

Token *tgen_ref_change_exec(Allocator *alc, Scope *scope, Value *on, int amount) {
    //
    Scope *sub = scope_init(alc, sct_default, scope, true);
    class_ref_change(alc, sub, on, amount);
    return tgen_exec(alc, sub, true);
}
