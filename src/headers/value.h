
#ifndef _H_VALUE
#define _H_VALUE

#include "structs.h"
#include "token.h"

typedef struct Value Value;
typedef struct VInt VInt;
typedef struct VFloat VFloat;
typedef struct VPtrv VPtrv;
typedef struct VPair VPair;
typedef struct VOp VOp;
typedef struct VFcall VFcall;
typedef struct VFuncPtr VFuncPtr;
typedef struct VClassPA VClassPA;

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

struct VPtrv {
    Value *on;
    Type *as;
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

#endif
