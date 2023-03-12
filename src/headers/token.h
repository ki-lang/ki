
#ifndef _H_TOKEN
#define _H_TOKEN

#include "structs.h"
#include "value.h"

typedef struct Token Token;
typedef struct TDecl TDecl;
typedef struct TIf TIf;
typedef struct TWhile TWhile;

struct Token {
    int type;
    void *item;
};

struct TDecl {
    Var *var;
    Value *value;
};
struct TIf {
    Value *cond;
    Scope *scope;
    TIf *else_if;
};
struct TWhile {
    Value *cond;
    Scope *scope;
};

#endif
