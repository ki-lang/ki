
#include "../all.h"

Class *class_init(Allocator *alc) {
    //
    Class *class = al(alc, sizeof(Class));
    class->type = ct_struct;
    class->size = 0;
    class->is_rc = true;
    class->must_deref = true;
    class->must_ref = true;
    class->is_signed = false;
    class->packed = true;
    class->is_generic_base = false;
    class->chunk_body = NULL;
    class->props = map_make(alc);
    class->funcs = map_make(alc);
    class->allow_math = false;

    class->func_ref = NULL;
    class->func_deref = NULL;
    class->func_deref_props = NULL;
    class->func_free = NULL;

    return class;
}
ClassProp *class_prop_init(Allocator *alc, Class *class, Type *type) {
    ClassProp *prop = al(alc, sizeof(ClassProp));
    prop->type = type;
    prop->index = class->props->keys->length;
    prop->value = NULL;
    prop->value_chunk = NULL;

    return prop;
}

bool class_check_size(Class *class) {
    // Calculate size (with alignment)
    if (class->type != ct_struct) {
        return true;
    }
    Build *b = class->fc->b;
    int size = 0;
    int largest = 0;
    int propc = class->props->values->length;
    for (int i = 0; i < propc; i++) {
        //
        ClassProp *prop = array_get_index(class->props->values, i);
        int prop_size = type_get_size(b, prop->type);
        if (prop_size == 0) {
            return false;
        }
        if (prop_size > largest) {
            largest = prop_size;
        }
        size += prop_size;
        //
        if (class->packed)
            continue;
        // Calculate padding
        int next_i = i + 1;
        if (next_i < propc) {
            ClassProp *prop = array_get_index(class->props->values, i);
            int next_size = type_get_size(b, prop->type);
            if (next_size == 0) {
                return false;
            }
            int rem = size % next_size;
            if (rem > 0) {
                // Add padding
                size += next_size - rem;
            }
        }
    }
    if (!class->packed) {
        int remain = size % largest;
        size += remain;
    }

    class->size = size;
    // printf("Size: %s | %d\n", class->display_name, size);

    return true;
}

Func *class_define_func(Fc *fc, Class *class, bool is_static, char *name_, Array *args, Type *rett) {
    //
    if (map_get(class->funcs, name_)) {
        return NULL;
    }

    char *name = dups(fc->alc, name_);

    char *gname = al(fc->alc, strlen(name) + strlen(class->gname) + 10);
    sprintf(gname, "%s__%s", class->gname, name);
    char *dname = al(fc->alc, strlen(name) + strlen(class->dname) + 10);
    sprintf(dname, "%s.%s", class->dname, name);

    Func *func = func_init(fc->alc);
    func->fc = fc;
    func->name = name;
    func->gname = gname;
    func->dname = dname;
    func->scope = scope_init(fc->alc, sct_func, fc->scope, true);
    func->scope->func = func;
    func->is_static = is_static;
    if (args)
        func->args = args;
    func->rett = rett;

    array_push(fc->funcs, func);
    map_set(class->funcs, name, func);

    if (!args && !is_static) {
        array_push(func->args, arg_init(fc->alc, "this", type_gen_class(fc->alc, class), false));
    }

    return func;
}

void class_generate_deref_props(Class *class) {
    //
    Func *func = class->func_deref_props;
    if (func->chunk_body)
        return;

    func_make_arg_decls(func);

    //
    Build *b = class->fc->b;
    Allocator *alc = b->alc;

    Scope *fscope = func->scope;
    Type *type_ptr = type_gen(b, alc, "ptr");
    Type *type_class = type_gen_class(alc, class);

    Arg *this_arg = array_get_index(func->args, 0);
    Decl *this_decl = this_arg->decl;
    Var *this_var = var_init(alc, this_decl, type_class);
    Value *this = value_init(alc, v_var, this_var, type_class);

    Array *props = class->props->values;
    for (int i = 0; i < props->length; i++) {
        ClassProp *prop = array_get_index(props, i);
        Class *pclass = prop->type->class;
        if (pclass && pclass->must_deref) {

            Value *pa = vgen_class_pa(alc, this, prop);

            Scope *scope = fscope;
            if (prop->type->nullable) {
                Value *is_null = vgen_compare(alc, class->fc->b, pa, vgen_null(alc, b), op_ne);
                Scope *sub = scope_init(alc, sct_default, scope, true);
                TIf *ift = tgen_tif(alc, is_null, sub, NULL);
                Token *t = token_init(alc, tkn_if, ift);
                array_push(scope->ast, t);
                scope = sub;
            }

            class_ref_change(b->alc, scope, pa, -1);
        }
    }
}
void class_generate_free(Class *class) {
    //
    Func *func = class->func_free;
    if (func->chunk_body)
        return;

    func_make_arg_decls(func);

    Build *b = class->fc->b;
    Allocator *alc = b->alc;

    Scope *fscope = func->scope;
    Type *type_ptr = type_gen(b, alc, "ptr");
    Type *type_class = type_gen_class(alc, class);

    Arg *this_arg = array_get_index(func->args, 0);
    Decl *this_decl = this_arg->decl;
    Var *this_var = var_init(alc, this_decl, type_class);
    Value *this = value_init(alc, v_var, this_var, type_class);

    // Call __before_free
    // let bff = this.funcs.get("__before_free") or value null;
    // verify bff {
    //     let ff = bff.func;
    //     let on = value_gen_func_ptr(ff, null);
    //     let values = Array<Value>.make();
    //     values.push(var_this);
    //     let fcall = value_gen_fcall(fc.b, fscope, on, values, type_gen_void());
    //     fscope.ast.push(Token{statement : fcall});
    // }

    // Call __deref_props (TODO)
    if (class->func_deref_props) {
        Value *on = vgen_fptr(alc, class->func_deref_props, NULL);
        Array *values = array_make(alc, 2);
        array_push(values, this);
        Value *fcall = vgen_fcall(alc, on, values, type_gen_void(alc));
        array_push(fscope->ast, token_init(alc, tkn_statement, fcall));
    }

    // Free mem
    Value *this_ptr = vgen_cast(alc, this, type_ptr);
    Func *ff = ki_get_func(b, "mem", "free");
    Value *on = vgen_fptr(alc, ff, NULL);
    Array *values = array_make(alc, 2);
    array_push(values, this_ptr);
    Value *fcall = vgen_fcall(alc, on, values, type_gen_void(alc));

    array_push(fscope->ast, token_init(alc, tkn_statement, fcall));
}

void class_ref_change(Allocator *alc, Scope *scope, Value *on, int amount) {
    //
    if (amount == 0)
        return;

    Type *type = on->rett;
    Class *class = type->class;
    Build *b = class->fc->b;

    if (type->nullable) {
        Value *is_null = vgen_compare(alc, class->fc->b, on, vgen_null(alc, b), op_ne);
        Scope *sub = scope_init(alc, sct_default, scope, true);
        TIf *ift = tgen_tif(alc, is_null, sub, NULL);
        Token *t = token_init(alc, tkn_if, ift);
        array_push(scope->ast, t);
        scope = sub;
    }

    if (amount < 0 && class->func_deref) {

        // Call __deref
        Value *fptr = vgen_fptr(alc, class->func_deref, NULL);
        Array *values = array_make(alc, 2);
        array_push(values, on);
        array_push(values, vgen_vint(alc, amount * -1, type_gen(class->fc->b, alc, "i32"), false));
        Value *fcall = vgen_fcall(alc, fptr, values, type_gen_void(alc));
        array_push(scope->ast, token_init(alc, tkn_statement, fcall));

    } else if (amount > 0 && class->func_ref) {

        // Call __ref
        Value *fptr = vgen_fptr(alc, class->func_ref, NULL);
        Array *values = array_make(alc, 2);
        array_push(values, on);
        array_push(values, vgen_vint(alc, amount, type_gen(class->fc->b, alc, "i32"), false));
        Value *fcall = vgen_fcall(alc, fptr, values, type_gen_void(alc));
        array_push(scope->ast, token_init(alc, tkn_statement, fcall));

    } else if (class->is_rc) {

        // _RC-- or _RC++
        Value *ir_on = vgen_ir_val(alc, on, on->rett);
        array_push(scope->ast, token_init(alc, tkn_ir_val, ir_on->item));

        ClassProp *prop = map_get(class->props, "_RC");
        Value *pa = vgen_class_pa(alc, ir_on, prop);

        Value *ir_pa = vgen_ir_assign_val(alc, pa, prop->type);
        array_push(scope->ast, token_init(alc, tkn_ir_assign_val, ir_pa->item));

        Value *ir_pa_load = value_init(alc, v_ir_load, ir_pa, prop->type);

        if (amount > 0) {

            Value *add = vgen_op(alc, class->fc->b, ir_pa_load, vgen_vint(alc, amount, prop->type, false), op_add, false);
            Token *as = tgen_assign(alc, ir_pa, add);
            array_push(scope->ast, as);

        } else if (amount < 0) {
            //
            Value *sub = vgen_op(alc, class->fc->b, ir_pa_load, vgen_vint(alc, amount * -1, prop->type, false), op_sub, false);

            Value *ir_sub = vgen_ir_val(alc, sub, prop->type);
            array_push(scope->ast, token_init(alc, tkn_ir_val, ir_sub->item));

            Value *is_zero = vgen_compare(alc, class->fc->b, ir_sub, vgen_vint(alc, 0, prop->type, false), op_eq);

            Scope *code = scope_init(alc, sct_default, scope, true);
            Scope *elif = scope_init(alc, sct_default, scope, true);
            // == 0 : Call free
            Value *fptr = vgen_fptr(alc, class->func_free, NULL);
            Array *values = array_make(alc, 2);
            array_push(values, ir_on);
            Value *fcall = vgen_fcall(alc, fptr, values, type_gen_void(alc));
            array_push(code->ast, token_init(alc, tkn_statement, fcall));

            // != 0 : else update _RC
            Token *as = tgen_assign(alc, ir_pa, ir_sub);
            array_push(elif->ast, as);

            //
            TIf *elift = tgen_tif(alc, NULL, elif, NULL);
            TIf *ift = tgen_tif(alc, is_zero, code, elift);
            Token *t = token_init(alc, tkn_if, ift);
            array_push(scope->ast, t);
        }
    }
}

void class_free_value(Allocator *alc, Scope *scope, Value *value) {

    Type *type = value->rett;
    Class *class = type->class;
    Build *b = class->fc->b;

    if (type->nullable) {
        Value *is_null = vgen_compare(alc, class->fc->b, value, vgen_null(alc, b), op_ne);
        Scope *sub = scope_init(alc, sct_default, scope, true);
        TIf *ift = tgen_tif(alc, is_null, sub, NULL);
        Token *t = token_init(alc, tkn_if, ift);
        array_push(scope->ast, t);
        scope = sub;
    }

    // Call __deref
    Value *fptr = vgen_fptr(alc, class->func_free, NULL);
    Array *values = array_make(alc, 2);
    array_push(values, value);
    Value *fcall = vgen_fcall(alc, fptr, values, type_gen_void(alc));
    array_push(scope->ast, token_init(alc, tkn_statement, fcall));
}