
#include "../all.h"

Value *vgen_vint(Allocator *alc, long int value, Type *type, bool force_type) {
    //
    VInt *item = al(alc, sizeof(VInt));
    item->value = value;
    item->force_type = force_type;
    return value_init(alc, v_vint, item, type);
}
Value *vgen_vfloat(Allocator *alc, Build *b, float value, bool force_type) {
    //
    VFloat *item = al(alc, sizeof(VFloat));
    item->value = value;
    item->force_type = force_type;
    return value_init(alc, v_float, item, type_gen(b, alc, "fxx"));
}

Value *vgen_ptrv(Allocator *alc, Value *on, Type *as) {
    //
    return value_init(alc, v_ptrv, on, as);
}

Value *vgen_op(Allocator *alc, Build *b, Value *left, Value *right, int op, bool is_ptr) {
    //
    VOp *item = al(alc, sizeof(VOp));
    item->left = left;
    item->right = right;
    item->op = op;

    Type *rett = left->rett;

    Value *res = value_init(alc, v_op, item, rett);
    if (is_ptr) {
        res = vgen_cast(alc, res, type_gen(b, alc, "ptr"));
    }

    return res;
}

Value *vgen_compare(Allocator *alc, Build *b, Value *left, Value *right, int op) {
    //
    VOp *item = al(alc, sizeof(VOp));
    item->left = left;
    item->right = right;
    item->op = op;
    return value_init(alc, v_compare, item, type_gen(b, alc, "bool"));
}

Value *vgen_fcall(Allocator *alc, Value *on, Array *values, Type *rett) {
    //
    VFcall *item = al(alc, sizeof(VFcall));
    item->on = on;
    item->args = values;
    return value_init(alc, v_fcall, item, rett);
}

Value *vgen_fptr(Allocator *alc, Func *func, Value *first_arg) {
    //
    VFuncPtr *item = al(alc, sizeof(VFuncPtr));
    item->func = func;
    item->first_arg = first_arg;
    return value_init(alc, v_fptr, item, type_gen_fptr(alc, func));
}

Value *vgen_class_pa(Allocator *alc, Value *on, ClassProp *prop) {
    //
    VClassPA *item = al(alc, sizeof(VClassPA));
    item->on = on;
    item->prop = prop;
    return value_init(alc, v_class_pa, item, prop->type);
}

Value *vgen_class_init(Allocator *alc, Class *class, Map *values) {
    //
    VClassInit *item = al(alc, sizeof(VClassInit));
    item->class = class;
    item->values = values;
    Type *rett = type_gen_class(alc, class);
    rett->owned = true;
    return value_init(alc, v_class_init, item, rett);
}

Value *vgen_cast(Allocator *alc, Value *val, Type *to_type) {
    //
    return value_init(alc, v_cast, val, to_type);
}

Value *vgen_null(Allocator *alc, Build *b) {
    Type *type = type_gen(b, alc, "ptr");
    type->type = type_null;
    type->nullable = true;
    type->class = NULL;
    return value_init(alc, v_null, NULL, type);
}

Value *vgen_or_break(Allocator *alc, Value *value, Scope *or_scope) {
    VOrBreak *item = al(alc, sizeof(VOrBreak));
    item->value = value;
    item->or_scope = or_scope;
    Type *rett = type_init(alc);
    *rett = *value->rett;
    rett->nullable = false;
    return value_init(alc, v_or_break, item, rett);
}

Value *vgen_or_value(Allocator *alc, Value *left, Value *right) {
    VOrValue *item = al(alc, sizeof(VOrValue));
    item->left = left;
    item->right = right;
    return value_init(alc, v_or_value, item, right->rett);
}

Value *vgen_and_or(Allocator *alc, Build *b, Value *left, Value *right, int op) {
    VOp *item = al(alc, sizeof(VOp));
    item->op = op;
    item->left = left;
    item->right = right;
    Type *rett = type_gen(b, alc, "bool");
    return value_init(alc, v_and_or, item, rett);
}

Value *vgen_ir_val(Allocator *alc, Value *value, Type *rett) {
    //
    IRVal *item = al(alc, sizeof(IRVal));
    item->value = value;
    item->ir_value = NULL;
    return value_init(alc, v_ir_val, item, rett);
}

Value *vgen_ir_assign_val(Allocator *alc, Value *value, Type *rett) {
    //
    IRAssignVal *item = al(alc, sizeof(IRAssignVal));
    item->value = value;
    item->ir_value = NULL;
    return value_init(alc, v_ir_assign_val, item, rett);
}