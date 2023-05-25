
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
typedef struct FCallOr FCallOr;
typedef struct VFuncPtr VFuncPtr;
typedef struct VClassPA VClassPA;
typedef struct VClassInit VClassInit;
typedef struct VArrayItem VArrayItem;
typedef struct VGlobal VGlobal;
typedef struct VOrBreak VOrBreak;
typedef struct VOrValue VOrValue;
typedef struct IRVal IRVal;
typedef struct IRAssignVal IRAssignVal;
typedef struct IRLoad IRLoad;
typedef struct ValueThenIRValue ValueThenIRValue;
typedef struct ValueAndExec ValueAndExec;
typedef struct VIncrDecr VIncrDecr;
typedef struct VFString VFString;

struct Value {
    int type;
    void *item;
    Type *rett;
    Array *issets;
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
    FCallOr * or ;
    UsageLine *ul;
};
struct VFuncPtr {
    Func *func;
    Value *first_arg;
};

struct VClassPA {
    Value *on;
    ClassProp *prop;
    char *llvm_val;
    Token *deref_token;
    Token *upref_token;
};
struct VClassInit {
    Class *class;
    Map *values;
    UsageLine *ul;
};
struct VArrayItem {
    Value *left;
    Value *right;
    char *llvm_val;
    Token *deref_token;
    Token *upref_token;
};
struct VGlobal {
    Global *g;
    char *llvm_val;
    Token *deref_token;
    Token *upref_token;
};
struct VOrBreak {
    Value *value;
    Scope *or_scope;
    Scope *else_scope;
    Scope *deref_scope;
};
struct VOrValue {
    Value *left;
    Value *right;
    Scope *value_scope;
    Scope *else_scope;
    Scope *deref_scope;
};
struct IRVal {
    Value *value;
    char *ir_value;
};
struct IRAssignVal {
    Value *value;
    char *ir_value;
};
struct ValueThenIRValue {
    Value *value;
    char *ir_value;
};
struct ValueAndExec {
    Value *value;
    Scope *exec_scope;
    bool before;
    bool enable_exec;
};
struct VIncrDecr {
    Value *value;
    bool is_incr;
};

struct FCallOr {
    Scope *scope;
    Scope *else_scope;
    Scope *deref_scope;
    Value *value;
    Decl *err_code_decl;
    Decl *err_msg_decl;
};
struct VFString {
    Array *parts;
    Array *values;
};

#endif
