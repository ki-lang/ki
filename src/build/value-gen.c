
#include "../all.h"

Value *vgen_vint(Allocator *alc, long int value, Type *type, bool force_type) {
    //
    VInt *item = al(alc, sizeof(VInt));
    item->value = value;
    item->force_type = force_type;
    return value_init(alc, v_vint, item, type);
}

Value *vgen_ptrv(Allocator *alc, Value *on, Type *as) {
    //
    VPtrv *vp = al(alc, sizeof(VPtrv));
    vp->on = on;
    vp->as = as;
    return value_init(alc, v_ptrv, vp, as);
}

Value *vgen_op(Allocator *alc, Value *left, Value *right, Type *rett) {
    //
    VPair *item = al(alc, sizeof(VPair));
    item->left = left;
    item->right = right;
    return value_init(alc, v_op, item, rett);
}

Value *vgen_compare(Allocator *alc, Build *b, Value *left, Value *right) {
    //
    VPair *item = al(alc, sizeof(VPair));
    item->left = left;
    item->right = right;
    return value_init(alc, v_compare, item, type_gen(b, alc, "bool"));
}

Value *vgen_fcall(Allocator *alc, Value *on, Array *args, Type *rett) {
    //
    VFcall *item = al(alc, sizeof(VFcall));
    item->on = on;
    item->args = args;
    return value_init(alc, v_fcall, item, rett);
}

Value *vgen_class_pa(Allocator *alc, Value *on, ClassProp *prop) {
    //
    VClassPA *item = al(alc, sizeof(VClassPA));
    item->on = on;
    item->prop = prop;
    return value_init(alc, v_class_pa, item, prop->type);
}
