
#include "../../headers/LLVM.h"

char *llvm_value(LB *b, Scope *scope, Value *v) {
    //
    Allocator *alc = b->alc;
    Fc *fc = b->fc;
    Build *build = fc->b;

    if (v->type == v_decl) {
        Decl *decl = v->item;
        char *var_val = decl->llvm_val;
        if (!var_val) {
            printf("Decl name: %s\n", decl->name);
            printf("Missing decl value (compiler bug)\n");
            raise(11);
        }
        if (!decl->is_mut) {
            return var_val;
        }
        return llvm_ir_load(b, decl->type, var_val);
    }
    if (v->type == v_global) {
        VGlobal *vg = v->item;
        Global *g = vg->g;
        char *res = llvm_ir_load(b, g->type, llvm_get_global(b, g->gname, g->type));
        vg->llvm_val = res;

        if (vg->upref_token) {
            Scope *sub = scope_init(alc, sct_default, scope, true);
            array_push(sub->ast, vg->upref_token);
            llvm_write_ast(b, sub);
        }
        return res;
    }
    if (v->type == v_ref) {
        Value *on = v->item;
        return llvm_value(b, scope, on);
    }
    if (v->type == v_ir_value) {
        return v->item;
    }
    if (v->type == v_vint) {
        VInt *vint = v->item;
        char *res = al(alc, 20);
        sprintf(res, "%ld", vint->value);
        return res;
    }
    if (v->type == v_float) {
        VFloat *vfl = v->item;
        char *res = al(alc, 20);
        sprintf(res, "%f", vfl->value);
        return res;
    }
    if (v->type == v_string) {
        return llvm_ir_string(b, v->item);
    }
    if (v->type == v_fstring) {
        VFString *vfs = v->item;
        Class *class = v->rett->class;
        Array *parts = vfs->parts;
        Array *values = vfs->values;
        int partc = parts->length;
        int valuec = values->length;

        Type *type = v->rett;
        char *ltype = llvm_type(b, v->rett);
        int base_len = 0;

        char nr[20];
        char partc_str[20];
        sprintf(partc_str, "%d", partc * type->bytes);
        char *part_adr = llvm_ir_stack_alloc(b, partc_str, "i64");
        for (int i = 0; i < partc; i++) {
            sprintf(nr, "%d", i);
            char *part_gep = llvm_ir_gep(b, ltype, part_adr, nr, "i64");
            char *str = array_get_index(parts, i);
            char *lstr = llvm_ir_string(b, str);
            llvm_ir_store(b, type, part_gep, lstr);
            base_len += strlen(str);
        }

        char valuec_str[20];
        sprintf(valuec_str, "%d", valuec * type->bytes);
        char *value_adr = llvm_ir_stack_alloc(b, valuec_str, "i64");
        for (int i = 0; i < valuec; i++) {
            sprintf(nr, "%d", i);
            char *value_gep = llvm_ir_gep(b, ltype, value_adr, nr, "i64");
            char *lvalue = llvm_value(b, scope, array_get_index(values, i));
            llvm_ir_store(b, type, value_gep, lvalue);
        }

        Func *func = map_get(class->funcs, "format");
        Array *format_values = array_make(alc, func->args->length + 1);
        array_push(format_values, vgen_vint(alc, base_len, type_gen(build, alc, "uxx"), false));
        array_push(format_values, value_init(alc, v_ir_raw_val, part_adr, type_gen(build, alc, "ptr")));
        array_push(format_values, vgen_vint(alc, partc, type_gen(build, alc, "uxx"), false));
        array_push(format_values, value_init(alc, v_ir_raw_val, value_adr, type_gen(build, alc, "ptr")));

        Array *lvals = llvm_ir_fcall_args(b, scope, format_values);
        char *on = llvm_ir_func_ptr(b, func);
        char *res = llvm_ir_func_call(b, on, lvals, ltype, NULL, vfs->line, vfs->col);

        return res;
    }
    if (v->type == v_null) {
        return "null";
    }
    if (v->type == v_ptrv) {
        char *lval = llvm_assign_value(b, scope, v);
        return llvm_ir_load(b, v->rett, lval);
    }
    if (v->type == v_getptr) {
        Value *val = v->item;
        return llvm_assign_value(b, scope, val);
    }
    if (v->type == v_ptrval) {
        Value *val = v->item;
        char *lval = llvm_value(b, scope, val);
        return llvm_ir_load(b, v->rett, lval);
    }
    if (v->type == v_op) {
        VOp *vop = v->item;
        int op = vop->op;
        char *lval1 = llvm_value(b, scope, vop->left);
        char *lval2 = llvm_value(b, scope, vop->right);
        Type *type = v->rett;
        char *ltype = llvm_type(b, type);
        char *var = llvm_var(b);

        Str *ir = llvm_b_ir(b);
        str_append_chars(ir, "  ");
        str_append_chars(ir, var);
        str_append_chars(ir, " = ");
        if (op == op_add) {
            str_append_chars(ir, "add ");
        } else if (op == op_sub) {
            str_append_chars(ir, "sub ");
        } else if (op == op_mul) {
            str_append_chars(ir, "mul ");
        } else if (op == op_div) {
            if (type->is_signed) {
                str_append_chars(ir, "sdiv ");
            } else {
                str_append_chars(ir, "udiv ");
            }
        } else if (op == op_mod) {
            if (type->is_signed) {
                str_append_chars(ir, "srem ");
            } else {
                str_append_chars(ir, "urem ");
            }
        } else if (op == op_bit_and) {
            str_append_chars(ir, "and ");
        } else if (op == op_bit_or) {
            str_append_chars(ir, "or ");
        } else if (op == op_bit_xor) {
            str_append_chars(ir, "xor ");
        } else if (op == op_shl) {
            str_append_chars(ir, "shl ");
        } else if (op == op_shr) {
            str_append_chars(ir, "lshr ");
        } else {
            die("Unknown LLVM math operation (compiler bug)");
        }
        str_append_chars(ir, ltype);
        str_append_chars(ir, " ");
        str_append_chars(ir, lval1);
        str_append_chars(ir, ", ");
        str_append_chars(ir, lval2);
        str_append_chars(ir, "\n");

        return var;
    }
    if (v->type == v_compare) {
        VOp *vop = v->item;
        int op = vop->op;
        char *lval1 = llvm_value(b, scope, vop->left);
        char *lval2 = llvm_value(b, scope, vop->right);
        Type *type = vop->left->rett;
        char *ltype = llvm_type(b, type);

        bool is_signed = type->is_signed;
        bool is_float = type->type == type_float;

        char *var_tmp = llvm_var(b);
        char *var_result = llvm_var(b);

        char *sign = "eq";
        if (op == op_ne) {
            sign = "ne";
        } else if (op == op_lt) {
            if (is_float) {
                sign = "olt";
            } else if (is_signed) {
                sign = "slt";
            } else {
                sign = "ult";
            }
        } else if (op == op_lte) {
            if (is_float) {
                sign = "ole";
            } else if (is_signed) {
                sign = "sle";
            } else {
                sign = "ule";
            }
        } else if (op == op_gt) {
            if (is_float) {
                sign = "ogt";
            } else if (is_signed) {
                sign = "sgt";
            } else {
                sign = "ugt";
            }
        } else if (op == op_gte) {
            if (is_float) {
                sign = "oge";
            } else if (is_signed) {
                sign = "sge";
            } else {
                sign = "uge";
            }
        }

        Str *ir = llvm_b_ir(b);
        str_append_chars(ir, "  ");
        str_append_chars(ir, var_tmp);
        str_append_chars(ir, " = icmp ");
        str_append_chars(ir, sign);
        str_append_chars(ir, " ");
        str_append_chars(ir, ltype);
        str_append_chars(ir, " ");
        str_append_chars(ir, lval1);
        str_append_chars(ir, ", ");
        str_append_chars(ir, lval2);
        str_append_chars(ir, "\n");

        str_append_chars(ir, "  ");
        str_append_chars(ir, var_result);
        str_append_chars(ir, " = zext i1 ");
        str_append_chars(ir, var_tmp);
        str_append_chars(ir, " to i8\n");

        return var_result;
    }
    if (v->type == v_fcall) {
        VFcall *fcall = v->item;
        char *on = llvm_value(b, scope, fcall->on);
        Array *values = llvm_ir_fcall_args(b, scope, fcall->args);
        char *res = llvm_ir_func_call(b, on, values, llvm_type(b, v->rett), fcall->or, fcall->line, fcall->col);
        if (fcall->ul) {
            fcall->ul->decl->llvm_val = res;
        }
        return res;
    }
    if (v->type == v_fptr) {
        VFuncPtr *fptr = v->item;
        return llvm_ir_func_ptr(b, fptr->func);
    }
    if (v->type == v_class_pa) {
        VClassPA *pa = v->item;
        if (pa->llvm_val) {
            if (pa->deref_token) {
                printf("File: %s\n", b->fc->path_ki);
                printf("Func: %s\n", scope->func->dname);
                die("LLVM IR: generated the same value twice (class property access)(compiler bug)");
            }
            return pa->llvm_val;
        }

        char *lval = llvm_assign_value(b, scope, v);
        char *res = llvm_ir_load(b, pa->prop->type, lval);
        if (pa->cache_llvm_val)
            pa->llvm_val = res;

        if (pa->upref_token) {
            Scope *sub = scope_init(alc, sct_default, scope, true);
            array_push(sub->ast, pa->upref_token);
            llvm_write_ast(b, sub);
        }
        return res;
    }
    if (v->type == v_class_init) {
        VClassInit *ci = v->item;
        Class *class = ci->class;
        Map *values = ci->values;

        Func *func = ki_get_func(build, "mem", "alloc");
        Value *fptr = vgen_fptr(alc, func, NULL);
        Array *alloc_values = array_make(alc, func->args->length + 1);
        Value *vint = vgen_vint(alc, class->size, type_gen(build, alc, "uxx"), false);
        array_push(alloc_values, vint);
        Value *fcall = vgen_fcall(alc, NULL, fptr, alloc_values, func->rett, NULL, 1, 1);
        Value *cast = vgen_cast(alc, fcall, v->rett);

        char *var_ob = llvm_value(b, scope, cast);
        for (int i = 0; i < values->keys->length; i++) {
            char *prop_name = array_get_index(values->keys, i);
            Value *val = array_get_index(values->values, i);
            ClassProp *prop = map_get(class->props, prop_name);
            Type *type = prop->type;
            char *ltype = llvm_type(b, type);
            char *lval = llvm_value(b, scope, val);
            char *pvar = llvm_var(b);

            char index[10];
            sprintf(index, "%d", prop->index);
            char bytes[10];
            int abytes = type->bytes;
            if (abytes > b->fc->b->ptr_size) {
                abytes = b->fc->b->ptr_size;
            }
            sprintf(bytes, "%d", abytes);

            llvm_check_defined(b, class);

            Str *ir = llvm_b_ir(b);
            str_append_chars(ir, "  ");
            str_append_chars(ir, pvar);
            str_append_chars(ir, " = getelementptr inbounds %struct.");
            str_append_chars(ir, class->gname);
            str_append_chars(ir, ", ptr ");
            str_append_chars(ir, var_ob);
            str_append_chars(ir, ", i32 0, i32 ");
            str_append_chars(ir, index);
            str_append_chars(ir, "\n");

            str_append_chars(ir, "  store ");
            str_append_chars(ir, ltype);
            str_append_chars(ir, " ");
            str_append_chars(ir, lval);
            str_append_chars(ir, ", ptr ");
            str_append_chars(ir, pvar);
            str_append_chars(ir, ", align ");
            str_append_chars(ir, bytes);
            str_append_chars(ir, "\n");
        }

        if (ci->ul) {
            ci->ul->decl->llvm_val = var_ob;
        }

        return var_ob;
    }
    if (v->type == v_cast) {
        Value *val = v->item;
        Type *from_type = val->rett;
        Type *to_type = v->rett;
        char *lval = llvm_value(b, scope, val);
        return llvm_ir_cast(b, lval, from_type, to_type);
    }
    if (v->type == v_swap) {
        VPair *item = v->item;
        Value *var = item->left;
        Value *val = item->right;
        char *lvar = llvm_assign_value(b, scope, var);
        char *res = llvm_ir_load(b, var->rett, lvar);

        char *lval = llvm_value(b, scope, val);
        llvm_ir_store(b, var->rett, lvar, lval);

        return res;
    }
    if (v->type == v_or_break) {
        VOrBreak *vob = v->item;
        Value *val = vob->value;
        Scope *sub = vob->or_scope;

        char *lval = llvm_value(b, scope, val);
        char *ltype = llvm_type(b, val->rett);
        char *isnull = llvm_ir_isnull_i1(b, ltype, lval);

        LLVMBlock *b_code = llvm_block_init_auto(b);
        LLVMBlock *b_else = llvm_block_init_auto(b);
        LLVMBlock *b_after = llvm_block_init_auto(b);

        if (vob->deref_scope)
            llvm_write_ast(b, vob->deref_scope);

        llvm_ir_cond_jump(b, llvm_b_ir(b), isnull, b_code, b_else);

        b->lfunc->block = b_code;
        llvm_write_ast(b, sub);
        if (!sub->did_return) {
            llvm_ir_jump(llvm_b_ir(b), b_after);
        }

        b->lfunc->block = b_else;
        llvm_write_ast(b, vob->else_scope);
        if (!vob->else_scope->did_return) {
            llvm_ir_jump(llvm_b_ir(b), b_after);
        }

        b->lfunc->block = b_after;
        return lval;
    }
    if (v->type == v_or_value) {
        VOrValue *orv = v->item;
        Value *left = orv->left;
        Value *right = orv->right;

        char *llval = llvm_value(b, scope, left);
        char *lltype = llvm_type(b, left->rett);
        char *isnull = llvm_ir_isnull_i1(b, lltype, llval);

        LLVMBlock *b_code = llvm_block_init_auto(b);
        LLVMBlock *b_else = llvm_block_init_auto(b);
        LLVMBlock *b_after = llvm_block_init_auto(b);

        char *current_block_name = b->lfunc->block->name;

        if (orv->deref_scope)
            llvm_write_ast(b, orv->deref_scope);

        llvm_ir_cond_jump(b, llvm_b_ir(b), isnull, b_code, b_else);

        b->lfunc->block = b_code;
        llvm_write_ast(b, orv->value_scope);
        char *rlval = llvm_value(b, scope, right);
        llvm_ir_jump(llvm_b_ir(b), b_after);

        char *last_block_code = b->lfunc->block->name;

        b->lfunc->block = b_else;
        llvm_write_ast(b, orv->else_scope);
        llvm_ir_jump(llvm_b_ir(b), b_after);

        char *last_block_else = b->lfunc->block->name;

        b->lfunc->block = b_after;
        Str *ir = b_after->ir;

        char *var_result = llvm_var(b);

        str_append_chars(ir, "  ");
        str_append_chars(ir, var_result);
        str_append_chars(ir, " = phi ");
        str_append_chars(ir, lltype);
        str_append_chars(ir, " [ ");
        str_append_chars(ir, llval);
        str_append_chars(ir, ", %");
        str_append_chars(ir, last_block_else);
        str_append_chars(ir, " ], [ ");
        str_append_chars(ir, rlval);
        str_append_chars(ir, ", %");
        str_append_chars(ir, last_block_code);
        str_append_chars(ir, " ]\n");

        return var_result;
    }
    if (v->type == v_and_or) {
        VOp *vop = v->item;
        bool is_or = vop->op == op_or;
        Value *left = vop->left;
        Value *right = vop->right;

        char *lval_left = llvm_value(b, scope, left);
        LLVMBlock *next_block = llvm_block_init_auto(b);
        LLVMBlock *after_block = llvm_block_init_auto(b);
        char *var_left = llvm_var(b);
        char *var_right = llvm_var(b);
        char *var_tmp = llvm_var(b);
        char *var_result = llvm_var(b);

        char *current_block_name = b->lfunc->block->name;

        Str *ir = llvm_b_ir(b);
        str_append_chars(ir, "  ");
        str_append_chars(ir, var_left);
        str_append_chars(ir, " = icmp ne i8 ");
        str_append_chars(ir, lval_left);
        str_append_chars(ir, ", 0\n");

        str_append_chars(ir, "  br i1 ");
        str_append_chars(ir, var_left);
        str_append_chars(ir, ", label %");
        if (is_or) {
            str_append_chars(ir, after_block->name);
            str_append_chars(ir, ", label %");
            str_append_chars(ir, next_block->name);
        } else {
            str_append_chars(ir, next_block->name);
            str_append_chars(ir, ", label %");
            str_append_chars(ir, after_block->name);
        }
        str_append_chars(ir, "\n");

        b->lfunc->block = next_block;
        char *lval_right = llvm_value(b, scope, right);
        Str *nir = llvm_b_ir(b);
        str_append_chars(nir, "  ");
        str_append_chars(nir, var_right);
        str_append_chars(nir, " = icmp ne i8 ");
        str_append_chars(nir, lval_right);
        str_append_chars(nir, ", 0\n");
        llvm_ir_jump(nir, after_block);

        LLVMBlock *next_block_last = b->lfunc->block;

        char *last_block = b->lfunc->block->name;

        b->lfunc->block = after_block;
        Str *air = after_block->ir;

        str_append_chars(air, "  ");
        str_append_chars(air, var_tmp);
        if (is_or) {
            str_append_chars(air, " = phi i1 [ true, %");
        } else {
            str_append_chars(air, " = phi i1 [ false, %");
        }
        str_append_chars(air, current_block_name);
        str_append_chars(air, " ], [ ");
        str_append_chars(air, var_right);
        str_append_chars(air, ", %");
        str_append_chars(air, last_block);
        str_append_chars(air, " ]\n");

        str_append_chars(air, "  ");
        str_append_chars(air, var_result);
        str_append_chars(air, " = zext i1 ");
        str_append_chars(air, var_tmp);
        str_append_chars(air, " to i8\n");

        return var_result;
    }
    if (v->type == v_upref_value) {
        char *lval = llvm_value(b, scope, v->item);
        Scope *sub = scope_init(alc, sct_default, scope, true);
        class_ref_change(alc, sub, value_init(alc, v_ir_value, lval, v->rett), 1, false);
        llvm_write_ast(b, sub);
        return lval;
    }
    if (v->type == v_ir_val) {
        IRVal *item = v->item;
        return item->ir_value;
    }
    if (v->type == v_ir_from) {
        Value *from = v->item;
        if (from->type == v_class_pa) {
            VClassPA *pa = from->item;
            if (!pa->llvm_val)
                die("LLVM IR error, missing llvm_val from v_class_pa (compiler bug)");
            return pa->llvm_val;
        } else if (from->type == v_global) {
            VGlobal *vg = from->item;
            if (!vg->llvm_val)
                die("LLVM IR error, missing llvm_val from v_global (compiler bug)");
            return vg->llvm_val;
        } else if (from->type == v_array_item) {
            VArrayItem *ai = from->item;
            if (!ai->llvm_val)
                die("LLVM IR error, missing llvm_val from v_array_item (compiler bug)");
            return ai->llvm_val;
        } else {
            die("LLVM IR error, cannot generate v_ir_val from this value (compiler bug)");
        }
    }
    if (v->type == v_ir_assign_val) {
        IRAssignVal *item = v->item;
        return item->ir_value;
    }
    if (v->type == v_ir_raw_val) {
        return v->item;
    }
    if (v->type == v_ir_load) {
        Value *val = v->item;
        char *lval = llvm_value(b, scope, val);
        return llvm_ir_load(b, v->rett, lval);
    }
    if (v->type == v_value_then_ir_value) {
        ValueThenIRValue *item = v->item;
        char *lval = item->ir_value;
        if (!lval) {
            lval = llvm_value(b, scope, item->value);
            item->ir_value = lval;
        }
        return lval;
    }
    if (v->type == v_value_and_exec) {
        ValueAndExec *item = v->item;
        if (item->before && item->enable_exec) {
            llvm_write_ast(b, item->exec_scope);
        }
        char *lval = llvm_value(b, scope, item->value);
        if (!item->before && item->enable_exec) {
            llvm_write_ast(b, item->exec_scope);
        }
        return lval;
    }
    if (v->type == v_incr_decr) {
        VIncrDecr *item = v->item;
        Value *val = item->value;
        bool is_incr = item->is_incr;
        char *lval = llvm_value(b, scope, val);
        char *retv = lval;
        char *lvarval = llvm_assign_value(b, scope, val);
        Type *type = v->rett;
        Type *vtype = type;
        char *ltype = llvm_type(b, type);
        char *lotype = ltype;

        bool is_ptr = type->type == type_ptr;

        if (is_ptr) {
            vtype = type_gen(b->fc->b, alc, "uxx");
            ltype = llvm_type(b, vtype);
            lval = llvm_ir_cast(b, lval, type, vtype);
        }

        Str *ir = llvm_b_ir(b);
        char *var_result = llvm_var(b);
        str_append_chars(ir, "  ");
        str_append_chars(ir, var_result);
        str_append_chars(ir, " = add nsw ");
        str_append_chars(ir, ltype);
        str_append_chars(ir, " ");
        str_append_chars(ir, lval);
        str_append_chars(ir, ", ");
        if (is_incr) {
            str_append_chars(ir, " 1\n");
        } else {
            str_append_chars(ir, " -1\n");
        }

        if (is_ptr) {
            var_result = llvm_ir_cast(b, var_result, vtype, type);
        }

        char bytes[10];
        sprintf(bytes, "%d", type->bytes);

        llvm_ir_store(b, type, lvarval, var_result);

        return retv;
    }
    if (v->type == v_stack_alloc) {
        Value *item = v->item;
        char *val = llvm_value(b, scope, item);
        Type *type = item->rett;
        char *ltype = llvm_type(b, type);
        return llvm_ir_stack_alloc(b, val, ltype);
    }
    if (v->type == v_atomicop) {
        VOp *vop = v->item;

        int op = vop->op;
        char *lval1 = llvm_assign_value(b, scope, vop->left);
        char *lval2 = llvm_value(b, scope, vop->right);
        char *ltype = llvm_type(b, v->rett);
        char *var = llvm_var(b);

        Str *ir = llvm_b_ir(b);
        str_append_chars(ir, "  ");
        str_append_chars(ir, var);
        str_append_chars(ir, " = atomicrmw ");
        if (op == op_add) {
            str_append_chars(ir, "add ");
        } else if (op == op_sub) {
            str_append_chars(ir, "sub ");
        } else if (op == op_bit_and) {
            str_append_chars(ir, "and ");
        } else if (op == op_bit_or) {
            str_append_chars(ir, "or ");
        } else if (op == op_bit_xor) {
            str_append_chars(ir, "xor ");
        } else {
            die("Unknown LLVM atomic operation (compiler bug)");
        }

        char bytes[10];
        int abytes = v->rett->bytes;
        if (abytes > b->fc->b->ptr_size) {
            abytes = b->fc->b->ptr_size;
        }
        sprintf(bytes, "%d", abytes);

        str_append_chars(ir, ltype);
        str_append_chars(ir, "* ");
        str_append_chars(ir, lval1);
        str_append_chars(ir, ", ");
        str_append_chars(ir, ltype);
        str_append_chars(ir, " ");
        str_append_chars(ir, lval2);
        str_append_chars(ir, " seq_cst, align ");
        str_append_chars(ir, bytes);
        str_append_chars(ir, "\n");

        return var;
    }
    if (v->type == v_scope) {
        // Value scope
        Scope *sub = v->item;
        Type *rett = v->rett;

        LLVMBlock *b_after = llvm_block_init_auto(b);

        sub->vscope->llvm_after_block = b_after;
        sub->vscope->lvar = llvm_alloca(b, rett);

        llvm_write_ast(b, sub);

        b->lfunc->block = b_after;

        return llvm_ir_load(b, rett, sub->vscope->lvar);
    }
    if (v->type == v_array_item) {
        VArrayItem *ai = v->item;
        Value *on = ai->left;
        Type *item_type = on->rett->array_of;
        char *result = llvm_assign_value(b, scope, v);
        char *res = llvm_ir_load(b, item_type, result);
        ai->llvm_val = res;

        if (ai->upref_token) {
            Scope *sub = scope_init(alc, sct_default, scope, true);
            array_push(sub->ast, ai->upref_token);
            llvm_write_ast(b, sub);
        }
        return res;
    }
    if (v->type == v_isset) {
        Value *on = v->item;
        char *ltype = llvm_type(b, on->rett);
        char *lval = llvm_value(b, scope, on);

        char *i1 = llvm_ir_notnull_i1(b, ltype, lval);

        Str *ir = llvm_b_ir(b);
        char *var = llvm_var(b);
        str_append_chars(ir, "  ");
        str_append_chars(ir, var);
        str_append_chars(ir, " = sext i1 ");
        str_append_chars(ir, i1);
        str_append_chars(ir, " to i8\n");

        return var;
    }

    return "???";
}

char *llvm_assign_value(LB *b, Scope *scope, Value *v) {
    //
    if (v->type == v_decl) {
        Decl *decl = v->item;
        return decl->llvm_val;
    }
    if (v->type == v_global) {
        VGlobal *vg = v->item;
        Global *g = vg->g;
        return llvm_get_global(b, g->gname, g->type);
    }
    if (v->type == v_ptrv) {
        VPair *pair = v->item;
        Value *on = pair->left;
        Value *index = pair->right;
        Type *as_type = v->rett;

        char *lval = llvm_value(b, scope, on);
        char *lindex = llvm_value(b, scope, index);
        char *lindex_type = llvm_type(b, index->rett);
        char *ltype = llvm_type_real(b, as_type);

        char *result = llvm_var(b);
        Str *ir = llvm_b_ir(b);
        str_append_chars(ir, "  ");
        str_append_chars(ir, result);
        str_append_chars(ir, " = getelementptr inbounds ");
        str_append_chars(ir, ltype);
        str_append_chars(ir, ", ptr ");
        str_append_chars(ir, lval);
        str_append_chars(ir, ", ");
        str_append_chars(ir, lindex_type);
        str_append_chars(ir, " ");
        str_append_chars(ir, lindex);
        str_append_chars(ir, "\n");

        return result;
    }
    if (v->type == v_class_pa) {
        VClassPA *pa = v->item;
        Value *on = pa->on;
        ClassProp *prop = pa->prop;
        Class *class = on->rett->class;
        Type *type = prop->type;
        char *lon = llvm_value(b, scope, on);
        return llvm_ir_class_prop_access(b, class, lon, prop);
    }
    if (v->type == v_ir_assign_val) {
        IRAssignVal *item = v->item;
        return item->ir_value;
    }
    if (v->type == v_array_item) {
        VArrayItem *ai = v->item;
        Value *on = ai->left;
        Value *index = ai->right;
        char *lon = llvm_value(b, scope, on);
        char *lindex = llvm_value(b, scope, index);

        Type *item_type = on->rett->array_of;
        char *l_item_type = llvm_type_real(b, item_type);
        char *l_index_type = llvm_type(b, index->rett);

        Str *ir = llvm_b_ir(b);
        char *result = llvm_var(b);
        str_append_chars(ir, "  ");
        str_append_chars(ir, result);
        str_append_chars(ir, " = getelementptr inbounds ");
        str_append_chars(ir, l_item_type);
        str_append_chars(ir, ", ptr ");
        str_append_chars(ir, lon);
        str_append_chars(ir, ", ");
        str_append_chars(ir, l_index_type);
        str_append_chars(ir, " ");
        str_append_chars(ir, lindex);
        str_append_chars(ir, "\n");

        return result;
    }

    die("LLVM : Cannot assign to this value");
    return "?A?";
}