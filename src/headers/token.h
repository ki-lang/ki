
#ifndef _H_TOKEN
#define _H_TOKEN

typedef struct Token Token;
typedef struct TIf TIf;
typedef struct TWhile TWhile;
typedef struct TExec TExec;
typedef struct Throw Throw;
typedef struct TEach TEach;
typedef struct TReturnVscope TReturnVscope;

#include "structs.h"
#include "value.h"

struct Token {
    int type;
    void *item;
};

struct TIf {
    Value *cond;
    Scope *scope;
    Scope *else_scope;
    Scope *deref_scope;
};
struct TWhile {
    Value *cond;
    Scope *scope;
};
struct TExec {
    Scope *scope;
    bool enable;
};
struct Throw {
    Func *func;
    int code;
};
struct TEach {
    Value *value;
    Scope *scope;
    Decl *decl_key;
    Decl *decl_value;
    int line;
    int col;
};
struct TReturnVscope {
    Value *value;
    Scope *scope;
};

#endif
