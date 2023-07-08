
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
    class->packed = false;
    class->is_generic_base = false;
    class->generic_names = NULL;
    class->generic_types = NULL;
    class->generics = NULL;
    class->chunk_body = NULL;
    class->props = map_make(alc);
    class->funcs = map_make(alc);
    class->allow_math = false;
    class->track_ownership = false;
    class->is_struct = false;
    class->can_iter = false;
    class->async = false;

    class->circular = false;
    class->circular_checked = false;

    class->func_ref = NULL;
    class->func_deref = NULL;
    class->func_deref_props = NULL;
    class->func_free = NULL;
    class->func_cc_check_props = NULL;
    class->func_cc_keep = NULL;
    class->func_iter_init = NULL;
    class->func_iter_get = NULL;
    class->func_before_free = NULL;
    class->func_leave_scope = NULL;

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
            if (next_size > b->ptr_size)
                next_size = b->ptr_size;
            int rem = size % next_size;
            if (rem > 0) {
                // Add padding
                size += next_size - rem;
            }
        }
    }
    if (!class->packed) {
        if (largest > b->ptr_size)
            largest = b->ptr_size;
        int rem = size % largest;
        size += rem;
    }

    class->size = size;
    // printf("Size: %s | %d\n", class->dname, size);

    return true;
}

Func *class_define_func(Fc *fc, Class *class, bool is_static, char *name_, Array *args, Type *rett, int line) {
    //
    if (map_get(class->funcs, name_)) {
        return NULL;
    }

    char *name = dups(fc->alc, name_);

    char *gname = al(fc->alc, strlen(name) + strlen(class->gname) + 10);
    sprintf(gname, "%s__%s", class->gname, name);
    char *dname = al(fc->alc, strlen(name) + strlen(class->dname) + 10);
    sprintf(dname, "%s.%s", class->dname, name);

    Func *func = func_init(fc->alc, fc->b);
    func->line = line;
    func->fc = fc;
    func->name = name;
    func->gname = gname;
    func->dname = dname;
    func->scope = scope_init(fc->alc, sct_func, class->scope, true);
    func->scope->func = func;
    func->is_static = is_static;
    if (args)
        func->args = args;
    if (rett)
        func->rett = rett;

    array_push(fc->funcs, func);
    map_set(class->funcs, name, func);

    if (!args && !is_static) {
        Arg *arg = arg_init(fc->alc, "this", type_gen_class(fc->alc, class));
        array_push(func->args, arg);
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
    Value *this = value_init(alc, v_decl, this_decl, type_class);

    Array *props = class->props->values;
    for (int i = 0; i < props->length; i++) {
        char *name = array_get_index(class->props->keys, i);
        ClassProp *prop = array_get_index(props, i);
        Class *pclass = prop->type->class;
        if (!prop->type->borrow && pclass && pclass->must_deref) {

            Value *pa = vgen_class_pa(alc, NULL, this, prop);
            Scope *scope = fscope;
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
    Value *this = value_init(alc, v_decl, this_decl, type_class);

    // Call __before_free
    if (class->func_before_free) {
        Value *on = vgen_fptr(alc, class->func_before_free, NULL);
        Array *values = array_make(alc, 2);
        array_push(values, this);
        Value *fcall = vgen_fcall(alc, NULL, on, values, b->type_void, NULL, 1, 1);
        array_push(fscope->ast, token_init(alc, tkn_statement, fcall));
    }

    // Call __deref_props
    if (class->func_deref_props) {
        Value *on = vgen_fptr(alc, class->func_deref_props, NULL);
        Array *values = array_make(alc, 2);
        array_push(values, this);
        Value *fcall = vgen_fcall(alc, NULL, on, values, b->type_void, NULL, 1, 1);
        array_push(fscope->ast, token_init(alc, tkn_statement, fcall));
    }

    // Free mem
    Value *this_ptr = vgen_cast(alc, this, type_ptr);
    Func *ff = ki_get_func(b, "mem", "free");
    Value *on = vgen_fptr(alc, ff, NULL);
    Array *values = array_make(alc, 2);
    array_push(values, this_ptr);
    Value *fcall = vgen_fcall(alc, NULL, on, values, b->type_void, NULL, 1, 1);

    array_push(fscope->ast, token_init(alc, tkn_statement, fcall));
}

void class_ref_change(Allocator *alc, Scope *scope, Value *on, int amount) {
    //
    if (amount == 0)
        return;

    Type *type = on->rett;
    Class *class = type->class;

    if (!class)
        return;

    Build *b = class->fc->b;

    if (!class->func_deref && !class->func_ref && !class->is_rc) {
        return;
    }

    if (type->nullable) {
        Value *is_null = vgen_compare(alc, class->fc->b, on, vgen_null(alc, b), op_ne);
        Scope *sub = scope_init(alc, sct_default, scope, true);
        TIf *ift = tgen_tif(alc, is_null, sub, NULL, NULL);
        Token *t = token_init(alc, tkn_if, ift);
        array_push(scope->ast, t);
        scope = sub;
    }

    if (amount < 0 && class->func_deref) {

        // Call __deref
        Value *fptr = vgen_fptr(alc, class->func_deref, NULL);
        Array *values = array_make(alc, 2);
        array_push(values, on);
        Value *fcall = vgen_fcall(alc, NULL, fptr, values, b->type_void, NULL, 1, 1);
        array_push(scope->ast, token_init(alc, tkn_statement, fcall));

    } else if (amount > 0 && class->func_ref) {

        // Call __ref
        Value *fptr = vgen_fptr(alc, class->func_ref, NULL);
        Array *values = array_make(alc, 2);
        array_push(values, on);
        Value *fcall = vgen_fcall(alc, NULL, fptr, values, b->type_void, NULL, 1, 1);
        array_push(scope->ast, token_init(alc, tkn_statement, fcall));

    } else if (class->is_rc) {

        // _RC-- or _RC++
        Value *ir_on = vgen_ir_val(alc, on, on->rett);
        array_push(scope->ast, token_init(alc, tkn_ir_val, ir_on->item));

        ClassProp *prop = map_get(class->props, "_RC");
        Value *pa = vgen_class_pa(alc, NULL, ir_on, prop);

        Value *ir_pa = vgen_ir_assign_val(alc, pa, prop->type);
        array_push(scope->ast, token_init(alc, tkn_ir_assign_val, ir_pa->item));

        if (amount > 0) {

            if (class->async) {
                Value *add = vgen_atomicop(alc, ir_pa, vgen_vint(alc, amount, prop->type, false), op_add);
                array_push(scope->ast, token_init(alc, tkn_statement, add));
            } else {
                Value *ir_pa_load = value_init(alc, v_ir_load, ir_pa, prop->type);
                Value *add = vgen_op(alc, class->fc->b, ir_pa_load, vgen_vint(alc, amount, prop->type, false), op_add, false);
                Token *as = tgen_assign(alc, ir_pa, add);
                array_push(scope->ast, as);
            }

        } else if (amount < 0) {
            //
            Value *ir_sub;
            Value *is_zero;

            // if (class->async) {
            Value *sub = vgen_atomicop(alc, ir_pa, vgen_vint(alc, amount * -1, prop->type, false), op_sub);
            ir_sub = vgen_ir_val(alc, sub, prop->type);
            array_push(scope->ast, token_init(alc, tkn_ir_val, ir_sub->item));
            is_zero = vgen_compare(alc, class->fc->b, ir_sub, vgen_vint(alc, 1, prop->type, false), op_eq);
            // } else {
            //     Value *ir_pa_load = value_init(alc, v_ir_load, ir_pa, prop->type);
            //     Value *sub = vgen_op(alc, class->fc->b, ir_pa_load, vgen_vint(alc, amount * -1, prop->type, false), op_sub, false);
            //     ir_sub = vgen_ir_val(alc, sub, prop->type);
            //     array_push(scope->ast, token_init(alc, tkn_ir_val, ir_sub->item));
            //     is_zero = vgen_compare(alc, class->fc->b, ir_sub, vgen_vint(alc, 0, prop->type, false), op_eq);
            // }

            Scope *code = scope_init(alc, sct_default, scope, true);
            Scope *elif = scope_init(alc, sct_default, scope, true);
            // == 0 : Call free
            Value *fptr = vgen_fptr(alc, class->func_free, NULL);
            Array *values = array_make(alc, 2);
            array_push(values, ir_on);
            Value *fcall = vgen_fcall(alc, NULL, fptr, values, b->type_void, NULL, 1, 1);
            array_push(code->ast, token_init(alc, tkn_statement, fcall));

            // != 0 : else update _RC
            // if (!class->async) {
            //     Token *as = tgen_assign(alc, ir_pa, ir_sub);
            //     array_push(elif->ast, as);
            // }

            //
            TIf *ift = tgen_tif(alc, is_zero, code, elif, NULL);
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
        TIf *ift = tgen_tif(alc, is_null, sub, NULL, NULL);
        Token *t = token_init(alc, tkn_if, ift);
        array_push(scope->ast, t);
        scope = sub;
    }

    // Call __deref
    Value *fptr = vgen_fptr(alc, class->func_free, NULL);
    Array *values = array_make(alc, 2);
    array_push(values, value);
    Value *fcall = vgen_fcall(alc, NULL, fptr, values, b->type_void, NULL, 1, 1);
    array_push(scope->ast, token_init(alc, tkn_statement, fcall));
}

void class_generate_generic_hash(Class *class, Array *types, char *buf) {
    //
}

Class *class_get_generic_class(Class *class, Array *types) {
    //
    char hash[33];
    strcpy(hash, "");

    Fc *fc = class->fc;
    Build *b = fc->b;
    Allocator *alc = b->alc;

    Str *hash_buf = b->str_buf;
    hash_buf->length = 0;
    char *type_buf = b->sbuf;
    for (int i = 0; i < types->length; i++) {
        if (i > 0)
            str_append_chars(hash_buf, ", ");
        Type *type = array_get_index(types, i);
        type_to_str(type, type_buf);
        str_append_chars(hash_buf, type_buf);
    }
    char *hash_content = str_to_chars(alc, hash_buf);
    simple_hash(hash_content, hash);

    Class *gclass = map_get(class->generics, hash);
    if (!gclass) {
        // Generate class

        sprintf(fc->sbuf, "%s[%s]", class->name, hash_content);
        char *name = dups(alc, fc->sbuf);
        sprintf(fc->sbuf, "%s[%s]", class->dname, hash_content);
        char *dname = dups(alc, fc->sbuf);
        sprintf(fc->sbuf, "%s__%s", class->gname, hash);
        char *gname = dups(alc, fc->sbuf);

        gclass = class_init(alc);
        *gclass = *class;
        gclass->name = name;
        gclass->dname = dname;
        gclass->gname = gname;

        Fc *new_fc = fc_init(b, gname, fc->nsc, fc->nsc->pkc, true);
        new_fc->chunk = chunk_clone(alc, fc->chunk);
        new_fc->scope->identifiers = fc->scope->identifiers;

        gclass->fc = new_fc;
        gclass->chunk_body = chunk_clone(alc, class->chunk_body);
        gclass->scope = scope_init(alc, sct_class, new_fc->scope, false);

        gclass->is_generic_base = false;
        gclass->generic_names = NULL;
        gclass->generic_types = types;
        gclass->generics = NULL;
        gclass->props = map_make(alc);
        gclass->funcs = map_make(alc);

        map_set(class->generics, hash, gclass);

        for (int i = 0; i < types->length; i++) {
            Type *type = array_get_index(types, i);
            char *name = array_get_index(class->generic_names, i);
            Idf *idf = idf_init(alc, idf_type);
            idf->item = type;
            map_set(gclass->scope->identifiers, name, idf);
        }

        array_push(new_fc->classes, gclass);

        Idf *idf = idf_init(alc, idf_class);
        idf->item = gclass;
        map_set(gclass->scope->identifiers, "CLASS", idf);

        // stage_2(new_fc);
        stage_2_class(new_fc, gclass);
        stage_2_class_defaults(new_fc, gclass);
        for (int i = 0; i < new_fc->funcs->length; i++) {
            Func *func = array_get_index(new_fc->funcs, i);
            if (!func->chunk_args)
                continue;
            if (b->verbose > 2) {
                printf("> Scan generic class func types: %s\n", func->dname);
            }
            stage_2_func(new_fc, func);
        }
        stage_2_class_type_checks(new_fc, gclass);
        stage_2_internals_gen(new_fc, gclass);
        class_check_size(gclass);
        stage_5(new_fc);
    }

    return gclass;
}

Array *read_generic_types(Fc *fc, Scope *scope, Class *class) {
    //
    char *token = fc->token;
    tok_expect(fc, "[", true, true);
    Array *types = array_make(fc->alc, class->generic_names->length + 1);
    while (true) {
        Type *type = read_type(fc, fc->alc, scope, true, true, rtc_default);
        array_push(types, type);

        tok(fc, token, true, true);
        if (strcmp(token, ",") != 0 && strcmp(token, "]") != 0) {
            sprintf(fc->sbuf, "Unexpected token '%s'", token);
            fc_error(fc);
        }
        if (strcmp(token, "]") == 0)
            break;
    }
    if (types->length < class->generic_names->length) {
        sprintf(fc->sbuf, "Missing types");
        fc_error(fc);
    }
    if (types->length > class->generic_names->length) {
        sprintf(fc->sbuf, "Too many types");
        fc_error(fc);
    }
    return types;
}

void class_generate_cc_check_props(Class *class) {
    //
    Func *func = class->func_cc_check_props;
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
    Value *this = value_init(alc, v_decl, this_decl, type_class);

    Value *ir_this = vgen_ir_val(alc, this, this->rett);
    array_push(fscope->ast, token_init(alc, tkn_ir_val, ir_this->item));

    Array *props = class->props->values;
    for (int i = 0; i < props->length; i++) {
        char *name = array_get_index(class->props->keys, i);
        ClassProp *prop1 = array_get_index(props, i);
        Class *pclass = prop1->type->class;

        if (!pclass || !pclass->circular)
            continue;

        Scope *scope = fscope;

        Value *ir_on = value_init(alc, v_ir_load, vgen_class_pa(alc, NULL, ir_this, prop1), prop1->type);

        if (prop1->type->nullable) {
            Value *is_null = vgen_compare(alc, class->fc->b, ir_on, vgen_null(alc, b), op_ne);
            Scope *sub = scope_init(alc, sct_default, scope, true);
            TIf *ift = tgen_tif(alc, is_null, sub, NULL, NULL);
            Token *t = token_init(alc, tkn_if, ift);
            array_push(scope->ast, t);
            scope = sub;
        }

        ClassProp *prop = map_get(pclass->props, "_RC_CHECK");
        if (!prop) {
            die("Compiler bug, missing _RC_CHECK");
        }
        Value *pa = vgen_class_pa(alc, NULL, ir_on, prop);
        Value *ir_pa = vgen_ir_assign_val(alc, pa, prop->type);
        array_push(scope->ast, token_init(alc, tkn_ir_assign_val, ir_pa->item));

        Value *ir_pa_load = value_init(alc, v_ir_load, ir_pa, prop->type);
        Value *add = vgen_op(alc, class->fc->b, ir_pa_load, vgen_vint(alc, 1, prop->type, false), op_add, false);
        Value *ir_add = vgen_ir_val(alc, add, prop->type);
        array_push(scope->ast, token_init(alc, tkn_ir_val, ir_add->item));

        Token *as = tgen_assign(alc, ir_pa, ir_add);
        array_push(scope->ast, as);
    }
}

void class_generate_cc_keep(Class *class) {
    //
    Func *func = class->func_cc_keep;
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
    Value *this = value_init(alc, v_decl, this_decl, type_class);

    Value *ir_this = vgen_ir_val(alc, this, this->rett);
    array_push(fscope->ast, token_init(alc, tkn_ir_val, ir_this->item));

    // this._CC_KEEP
    ClassProp *prop = map_get(class->props, "_CC_KEEP");
    Value *pa = vgen_class_pa(alc, NULL, ir_this, prop);
    Value *ir_pa = vgen_ir_assign_val(alc, pa, prop->type);
    array_push(fscope->ast, token_init(alc, tkn_ir_assign_val, ir_pa->item));
    Value *ir_pa_load = value_init(alc, v_ir_load, ir_pa, prop->type);

    // if (this._CC_KEEP == 1) return;
    Scope *sub = scope_init(alc, sct_default, fscope, true);
    array_push(sub->ast, tgen_return(alc, fscope, NULL));

    Value *cmp = vgen_compare(alc, class->fc->b, ir_pa_load, vgen_vint(alc, 1, prop->type, false), op_eq);
    TIf *ift = tgen_tif(alc, cmp, sub, NULL, NULL);
    Token *t = token_init(alc, tkn_if, ift);
    array_push(fscope->ast, t);

    // Set __CC_KEEP to 1
    Token *as = tgen_assign(alc, ir_pa, vgen_vint(alc, 1, prop->type, false));
    array_push(fscope->ast, as);

    // call __cc_keep() on circular props
    Array *props = class->props->values;
    for (int i = 0; i < props->length; i++) {
        char *name = array_get_index(class->props->keys, i);
        ClassProp *prop = array_get_index(props, i);
        Class *pclass = prop->type->class;

        if (!pclass || !pclass->circular)
            continue;

        Value *ir_on = value_init(alc, v_ir_load, vgen_class_pa(alc, NULL, ir_this, prop), prop->type);

        Scope *scope = fscope;
        if (prop->type->nullable) {
            Value *is_null = vgen_compare(alc, class->fc->b, ir_on, vgen_null(alc, b), op_ne);
            Scope *sub = scope_init(alc, sct_default, scope, true);
            TIf *ift = tgen_tif(alc, is_null, sub, NULL, NULL);
            Token *t = token_init(alc, tkn_if, ift);
            array_push(scope->ast, t);
            scope = sub;
        }

        Value *fptr = vgen_fptr(alc, pclass->func_cc_keep, NULL);
        Array *values = array_make(alc, 2);
        array_push(values, ir_on);
        Value *fcall = vgen_fcall(alc, NULL, fptr, values, b->type_void, NULL, 1, 1);
        array_push(scope->ast, token_init(alc, tkn_statement, fcall));
    }
}