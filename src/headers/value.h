
#ifndef _H_VALUE
#define _H_VALUE

#include "structs.h"
#include "token.h"

typedef struct Value Value;
typedef struct VInt VInt;
typedef struct VFloat VFloat;
typedef struct VPair VPair;
typedef struct VOp VOp;
typedef struct VFcall VFcall;
typedef struct VFuncPtr VFuncPtr;
typedef struct VClassPA VClassPA;
typedef struct VClassInit VClassInit;
typedef struct VOrBreak VOrBreak;
typedef struct VOrValue VOrValue;

struct Value {
    int type;
    void *item;
    Type *rett;
};

struct VInt {
    long int value;
    bool force_type;
};

struct VFloat {
    float value;
    bool force_type;
};

struct VPair {
    Value *left;
    Value *right;
};

struct VOp {
    Value *left;
    Value *right;
    int op;
};

struct VFcall {
    Value *on;
    Array *args;
};
struct VFuncPtr {
    Func *func;
    Value *first_arg;
};

struct VClassPA {
    Value *on;
    ClassProp *prop;
};
struct VClassInit {
    Class *class;
    Map *values;
};
struct VOrBreak {
    Value *value;
    Scope *or_scope;
};
struct VOrValue {
    Value *left;
    Value *right;
};

#endif
