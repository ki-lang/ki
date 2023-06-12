
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

Value *vgen_ptrv(Allocator *alc, Value *on, Type *as, Value *index) {
    //
    VPair *item = al(alc, sizeof(VPair));
    item->left = on;
    item->right = index;
    return value_init(alc, v_ptrv, item, as);
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

Value *vgen_fcall(Allocator *alc, Scope *scope, Value *on, Array *values, Type *rett, FCallOr * or, int line, int col) {
    //
    VFcall *item = al(alc, sizeof(VFcall));
    item->on = on;
    item->args = values;
    item->or = or ;
    item->ul = NULL;
    item->line = line;
    item->col = col;

    if (scope && type_tracks_ownership(rett)) {
        Decl *decl = decl_init(alc, scope, "KI_GENERATED_TEMP_VAR", rett, NULL, false);
        item->ul = usage_line_init(alc, scope, decl);
    }
    return value_init(alc, v_fcall, item, rett);
}

Value *vgen_fptr(Allocator *alc, Func *func, Value *first_arg) {
    //
    VFuncPtr *item = al(alc, sizeof(VFuncPtr));
    item->func = func;
    item->first_arg = first_arg;
    return value_init(alc, v_fptr, item, type_gen_fptr(alc, func));
}

Value *vgen_class_pa(Allocator *alc, Scope *scope, Value *on, ClassProp *prop) {
    //
    value_disable_upref_deref(on);

    VClassPA *item = al(alc, sizeof(VClassPA));
    item->on = on;
    item->prop = prop;
    item->llvm_val = NULL;
    item->deref_token = NULL;
    item->upref_token = NULL;

    Type *prop_type = prop->type;
    Value *res = value_init(alc, v_class_pa, item, prop_type);

    if (scope && type_tracks_ownership(prop_type)) {
        Type *rett = type_clone(alc, prop_type);
        res->rett = rett;

        Value *from = vgen_ir_from(alc, res);
        item->deref_token = tgen_ref_change_exec(alc, scope, from, -1);
        item->upref_token = tgen_ref_change_exec(alc, scope, from, 1);
        scope_add_defer_token(alc, scope, item->deref_token);
    }
    return res;
}

Value *vgen_ir_from(Allocator *alc, Value *from) {
    //
    return value_init(alc, v_ir_from, from, from->rett);
}

Value *vgen_class_init(Allocator *alc, Scope *scope, Class *class, Map *values) {
    //
    VClassInit *item = al(alc, sizeof(VClassInit));
    item->class = class;
    item->values = values;
    item->ul = NULL;

    Type *rett = type_gen_class(alc, class);
    rett->strict_ownership = true;

    if (scope) {
        Decl *decl = decl_init(alc, scope, "KI_GENERATED_TEMP_VAR", rett, NULL, false);
        item->ul = usage_line_init(alc, scope, decl);
    }

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

Value *vgen_or_break(Allocator *alc, Value *value, Scope *or_scope, Scope *else_scope, Scope *deref_scope) {
    VOrBreak *item = al(alc, sizeof(VOrBreak));
    item->value = value;
    item->or_scope = or_scope;
    item->else_scope = else_scope;
    item->deref_scope = deref_scope;
    Type *rett = type_init(alc);
    *rett = *value->rett;
    rett->nullable = false;
    return value_init(alc, v_or_break, item, rett);
}

Value *vgen_or_value(Allocator *alc, Value *left, Value *right, Scope *value_scope, Scope *else_scope, Scope *deref_scope) {
    VOrValue *item = al(alc, sizeof(VOrValue));
    item->left = left;
    item->right = right;
    item->value_scope = value_scope;
    item->else_scope = else_scope;
    item->deref_scope = deref_scope;
    return value_init(alc, v_or_value, item, right->rett);
}

Value *vgen_and_or(Allocator *alc, Build *b, Value *left, Value *right, int op) {
    VOp *item = al(alc, sizeof(VOp));
    item->op = op;
    item->left = left;
    item->right = right;
    Type *rett = type_gen(b, alc, "bool");

    Value *result = value_init(alc, v_and_or, item, rett);

    // merge issets when using '&&'
    if (op == op_and && (left->issets || right->issets)) {
        Array *issets = array_make(alc, 4);
        if (left->issets) {
            Array *prev = left->issets;
            for (int i = 0; i < prev->length; i++) {
                array_push(issets, array_get_index(prev, i));
            }
        }
        if (right->issets) {
            Array *prev = right->issets;
            for (int i = 0; i < prev->length; i++) {
                array_push(issets, array_get_index(prev, i));
            }
        }
        result->issets = issets;
    }

    return result;
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

Value *vgen_value_and_exec(Allocator *alc, Value *value, Scope *exec_scope, bool before_value, bool enable_exec) {
    //
    ValueAndExec *item = al(alc, sizeof(ValueAndExec));
    item->value = value;
    item->exec_scope = exec_scope;
    item->before = before_value;
    item->enable_exec = enable_exec;
    return value_init(alc, v_value_and_exec, item, value->rett);
}

Value *vgen_value_then_ir_value(Allocator *alc, Value *value) {
    //
    ValueThenIRValue *item = al(alc, sizeof(ValueThenIRValue));
    item->value = value;
    item->ir_value = NULL;
    return value_init(alc, v_value_then_ir_value, item, value->rett);
}

Value *vgen_incr_decr(Allocator *alc, Value *on, bool is_incr) {
    //
    VIncrDecr *item = al(alc, sizeof(VIncrDecr));
    item->value = on;
    item->is_incr = is_incr;
    return value_init(alc, v_incr_decr, item, on->rett);
}

Value *vgen_atomicop(Allocator *alc, Value *left, Value *right, int op) {
    //
    VOp *item = al(alc, sizeof(VOp));
    item->left = left;
    item->right = right;
    item->op = op;
    return value_init(alc, v_atomicop, item, left->rett);
}

Value *vgen_array_item(Allocator *alc, Scope *scope, Value *on, Value *index) {
    //
    value_disable_upref_deref(on);

    VArrayItem *item = al(alc, sizeof(VArrayItem));
    item->left = on;
    item->right = index;
    item->llvm_val = NULL;
    item->deref_token = NULL;
    item->upref_token = NULL;

    Type *type = on->rett->array_of;
    Value *res = value_init(alc, v_array_item, item, type);

    if (scope && type_tracks_ownership(type)) {
        Type *rett = type_clone(alc, type);
        res->rett = rett;

        Value *from = vgen_ir_from(alc, res);
        item->deref_token = tgen_ref_change_exec(alc, scope, from, -1);
        item->upref_token = tgen_ref_change_exec(alc, scope, from, 1);
        scope_add_defer_token(alc, scope, item->deref_token);
    }
    return res;
}

Value *vgen_swap(Allocator *alc, Value *var, Value *with) {
    //
    VPair *item = al(alc, sizeof(VPair));
    item->left = var;
    item->right = with;
    return value_init(alc, v_swap, item, var->rett);
}