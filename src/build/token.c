
#include "../all.h"

Token *token_init(Allocator *alc, int type, void *item) {
    //
    Token *t = al(alc, sizeof(Token));
    t->type = type;
    t->item = item;

    return t;
}

TIf *tgen_tif(Allocator *alc, Value *cond, Scope *scope, TIf *else_if) {
    //
    TIf *tif = al(alc, sizeof(TIf));
    tif->cond = cond;
    tif->scope = scope;
    tif->else_if = else_if;
    return tif;
}

Token *tgen_declare(Allocator *alc, Var *var, Value *val) {
    //
    TDecl *decl = al(alc, sizeof(TDecl));
    decl->var = var;
    decl->value = val;
    return token_init(alc, tkn_declare, decl);
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
    fscope->did_return = true;
    return token_init(alc, tkn_return, retv);
}
