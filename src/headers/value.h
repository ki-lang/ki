
typedef struct Value Value;
typedef struct VInt VInt;
typedef struct VPtrv VPtrv;
typedef struct VPair VPair;
typedef struct VFcall VFcall;
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

struct VPtrv {
    Value *on;
    Type *as;
};

struct VPair {
    Value *left;
    Value *right;
};

struct VFcall {
    Value *on;
    Array *args;
};

struct VClassPA {
    Value *on;
    ClassProp *prop;
};
