
#include "../../headers/LLVM.h"

void llvm_if(LB *b, Scope *scope, TIf *ift);

void llvm_write_ast(LB *b, Scope *scope) {
    //

    Allocator *alc = b->alc;

    Array *ast = scope->ast;
    for (int i = 0; i < ast->length; i++) {
        Token *t = array_get_index(ast, i);

        if (t->type == tkn_declare) {
            Decl *decl = t->item;
            Value *val = decl->value;

            char *lval = llvm_value(b, scope, val);

            if (!decl->is_mut) {
                decl->llvm_val = lval;
                continue;
            }

            char *lvar = llvm_alloca(b, decl->type);
            decl->llvm_val = lvar;
            llvm_ir_store(b, decl->type, lvar, lval);
            continue;
        }
        if (t->type == tkn_assign) {
            VPair *pair = t->item;
            Value *left = pair->left;
            Type *lt = left->rett;
            char *lvar = llvm_assign_value(b, scope, left);
            char *lval = llvm_value(b, scope, pair->right);
            llvm_ir_store(b, lt, lvar, lval);
            continue;
        }
        if (t->type == tkn_ir_val) {
            IRVal *item = t->item;
            item->ir_value = llvm_value(b, scope, item->value);
            continue;
        }
        if (t->type == tkn_ir_assign_val) {
            IRAssignVal *item = t->item;
            item->ir_value = llvm_assign_value(b, scope, item->value);
            continue;
        }
        if (t->type == tkn_return) {
            Value *val = t->item;
            LLVMFunc *lfunc = b->lfunc;
            Str *ir = lfunc->block->ir;

            char *save_vn = lfunc->stack_save_vn;
            if (save_vn) {
                str_append_chars(ir, "  call void @llvm.stackrestore(i8* ");
                str_append_chars(ir, save_vn);
                str_append_chars(ir, ")\n");
            }
            char *lval = "void";
            if (val) {
                lval = llvm_value(b, scope, val);
                char *ltype = llvm_type(b, val->rett);
                char *buf = al(b->alc, strlen(lval) + 2 + strlen(ltype));
                strcpy(buf, ltype);
                strcat(buf, " ");
                strcat(buf, lval);
                lval = buf;
            }
            Str *rir = llvm_b_ir(b);
            str_append_chars(rir, "  ret ");
            str_append_chars(rir, lval);
            str_append_chars(rir, "\n");
            continue;
        }
        if (t->type == tkn_throw) {
            Throw *item = t->item;
            Type *err_code_type = type_gen(b->fc->b, b->alc, "i32");
            char code[20];
            sprintf(code, "%d", item->code);
            llvm_ir_store(b, err_code_type, "@ki_err_code_buffer", code);

            Type *rett = item->func->rett;
            char *ltype = llvm_type(b, rett);

            Str *ir = llvm_b_ir(b);
            str_append_chars(ir, "  ret ");
            str_append_chars(ir, ltype);
            if (!type_is_void(item->func->rett)) {
                str_append_chars(ir, " ");
                str_append_chars(ir, item->func->rett->ptr_depth > 0 ? "null" : "0");
            }
            str_append_chars(ir, "\n");
            continue;
        }
        if (t->type == tkn_if) {
            TIf *ift = t->item;
            llvm_if(b, scope, ift);
            continue;
        }
        if (t->type == tkn_while) {
            TWhile *w = t->item;

            Value *cond = w->cond;
            Scope *sub = w->scope;

            Str *ir_a = b->ir_attr;
            char *loop_attr = llvm_attr(b);
            str_append_chars(ir_a, loop_attr);
            str_append_chars(ir_a, " = distinct !{");
            str_append_chars(ir_a, loop_attr);
            str_append_chars(ir_a, ", !0}\n");

            LLVMBlock *b_cond = llvm_block_init_auto(b);
            LLVMBlock *b_code = llvm_block_init_auto(b);
            LLVMBlock *b_after = llvm_block_init_auto(b);
            LLVMBlock *prev_cond = b->while_cond;
            LLVMBlock *prev_after = b->while_after;
            char *prev_attr = b->loop_attr;
            b->while_cond = b_cond;
            b->while_after = b_after;
            b->loop_attr = loop_attr;

            llvm_ir_jump(llvm_b_ir(b), b_cond);

            b->lfunc->block = b_cond;
            char *lcond = llvm_value(b, scope, cond);
            Str *ir_c = llvm_b_ir(b);
            char *lcond_i1 = llvm_ir_bool_i1(b, ir_c, lcond);
            llvm_ir_cond_jump(b, ir_c, lcond_i1, b_code, b_after);

            b->lfunc->block = b_code;
            llvm_write_ast(b, sub);
            if (!sub->did_return) {
                llvm_ir_jump_loop(b, b_cond);
            }

            b->lfunc->block = b_after;
            b->loop_attr = prev_attr;
            b->while_cond = prev_cond;
            b->while_after = prev_after;
            continue;
        }
        if (t->type == tkn_each) {
            TEach *item = t->item;

            Value *on = item->value;
            char *lon = llvm_value(b, scope, on);
            char *lon_type = llvm_type(b, on->rett);

            Scope *sub = item->scope;
            Decl *decl_key = item->decl_key;
            Decl *decl_value = item->decl_value;

            Type *type = on->rett;
            Class *class = type->class;
            Func *f_init = map_get(class->funcs, "__each_init");
            Func *f_get = map_get(class->funcs, "__each");
            char *lf_init = llvm_ir_func_ptr(b, f_init);
            char *lf_get = llvm_ir_func_ptr(b, f_get);
            Type *key_type = f_init->rett;
            char *key_ltype = llvm_type(b, key_type);
            Type *value_type = f_get->rett;
            char *value_ltype = llvm_type(b, value_type);

            Arg *f_init_0 = array_get_index(f_init->args, 0);
            Arg *f_get_0 = array_get_index(f_get->args, 0);

            char key_ltype_pointer[200];
            strcpy(key_ltype_pointer, key_ltype);
            strcat(key_ltype_pointer, "*");

            char *loop_attr = llvm_attr(b);
            Str *ir_a = b->ir_attr;
            str_append_chars(ir_a, loop_attr);
            str_append_chars(ir_a, " = distinct !{");
            str_append_chars(ir_a, loop_attr);
            str_append_chars(ir_a, ", !0}\n");
            char *prev_attr = b->loop_attr;

            LLVMBlock *b_cond = llvm_block_init_auto(b);
            LLVMBlock *b_code = llvm_block_init_auto(b);
            LLVMBlock *b_after = llvm_block_init_auto(b);

            LLVMBlock *prev_cond = b->while_cond;
            LLVMBlock *prev_after = b->while_after;
            b->while_cond = b_cond;
            b->while_after = b_after;
            b->loop_attr = loop_attr;

            // Init key
            char *next_key_var = llvm_alloca(b, key_type);
            Array *key_init_args = array_make(alc, 2);
            array_push(key_init_args, llvm_ir_fcall_arg(b, lon, lon_type));
            // Referencing
            Scope *upref_init_0 = scope_init(alc, sct_default, scope, true);
            if (!f_init_0->type->borrow) {
                class_ref_change(alc, upref_init_0, value_init(alc, v_ir_value, lon, on->rett), 1);
            }
            llvm_write_ast(b, upref_init_0);
            // Call func
            char *next_key_val = llvm_ir_func_call(b, lf_init, key_init_args, key_ltype, NULL);
            llvm_ir_store(b, f_init->rett, next_key_var, next_key_val);
            //
            llvm_ir_jump(llvm_b_ir(b), b_cond);

            // Start loop (get value & next key)
            b->lfunc->block = b_cond;
            // Call __each
            char *key_val = llvm_ir_load(b, f_init->rett, next_key_var);
            // Referencing
            Scope *upref_get = scope_init(alc, sct_default, scope, true);
            if (!f_get_0->type->borrow) {
                class_ref_change(alc, upref_get, value_init(alc, v_ir_value, lon, on->rett), 1);
            }
            llvm_write_ast(b, upref_get);

            //
            Array *get_args = array_make(alc, 2);
            array_push(get_args, llvm_ir_fcall_arg(b, lon, lon_type));
            array_push(get_args, llvm_ir_fcall_arg(b, next_key_var, key_ltype_pointer));
            char *lval = llvm_ir_func_call(b, lf_get, get_args, value_ltype, NULL);
            // Check error
            Type *err_code_type = type_gen(b->fc->b, b->alc, "i32");
            char *load_err = llvm_ir_load(b, err_code_type, "@ki_err_code_buffer");
            char *iszero = llvm_ir_iszero_i1(b, "i32", load_err);
            llvm_ir_cond_jump(b, llvm_b_ir(b), iszero, b_code, b_after);
            // Set variable values

            decl_key->llvm_val = key_val;
            decl_value->llvm_val = lval;

            // Loop code
            b->lfunc->block = b_code;
            llvm_write_ast(b, sub);
            if (!sub->did_return) {
                llvm_ir_jump_loop(b, b_cond);
            }

            b->lfunc->block = b_after;
            llvm_ir_store(b, err_code_type, "@ki_err_code_buffer", "0");

            b->loop_attr = prev_attr;
            b->while_cond = prev_cond;
            b->while_after = prev_after;
            continue;
        }
        if (t->type == tkn_break) {
            LLVMBlock *after_block = b->while_after;
            llvm_ir_jump(llvm_b_ir(b), after_block);
            continue;
        }
        if (t->type == tkn_continue) {
            LLVMBlock *cond_block = b->while_cond;
            llvm_ir_jump_loop(b, cond_block);
            continue;
        }
        if (t->type == tkn_statement) {
            Value *val = t->item;
            char *lval = llvm_value(b, scope, val);
            continue;
        }
        if (t->type == tkn_exec) {
            TExec *exec = t->item;
            if (exec->enable)
                llvm_write_ast(b, exec->scope);
            continue;
        }
        if (t->type == tkn_vscope_return) {
            TReturnVscope *ret = t->item;
            Scope *vscope = ret->scope;
            Value *retv = ret->value;
            Type *rett = vscope->vscope->rett;
            char *lvar = vscope->vscope->lvar;
            char *lval = llvm_value(b, scope, retv);
            llvm_ir_store(b, rett, lvar, lval);
            Str *ir = llvm_b_ir(b);
            llvm_ir_jump(ir, vscope->vscope->llvm_after_block);
            continue;
        }
    }

    if (scope->did_exit_function) {
        Func *func = scope->func;
        if (func) {
            Str *ir = llvm_b_ir(b);
            Type *rett = func->rett;
            if (type_is_void(rett)) {
                str_append_chars(ir, "  ret void\n");
            } else {
                char *ltype = llvm_type(b, rett);
                str_append_chars(ir, "  ret ");
                str_append_chars(ir, ltype);
                str_append_chars(ir, " 0\n");
            }
        }
    }
}

void llvm_if(LB *b, Scope *scope, TIf *ift) {
    //
    Value *cond = ift->cond;
    Scope *sub = ift->scope;
    Scope *else_scope = ift->else_scope;
    Scope *deref_scope = ift->deref_scope;

    LLVMBlock *code_block = llvm_block_init_auto(b);
    LLVMBlock *else_block = llvm_block_init_auto(b);
    LLVMBlock *after = llvm_block_init_auto(b);

    char *lcond = llvm_value(b, scope, cond);

    if (deref_scope)
        llvm_write_ast(b, deref_scope);

    Str *ir = llvm_b_ir(b);
    char *lcond_i1 = llvm_ir_bool_i1(b, ir, lcond);
    llvm_ir_cond_jump(b, ir, lcond_i1, code_block, else_block);

    b->lfunc->block = code_block;
    llvm_write_ast(b, sub);
    if (!sub->did_return) {
        Str *ir = llvm_b_ir(b);
        llvm_ir_jump(ir, after);
    }

    b->lfunc->block = else_block;
    if (else_scope)
        llvm_write_ast(b, else_scope);
    if (!else_scope || !else_scope->did_return) {
        Str *ir = llvm_b_ir(b);
        llvm_ir_jump(ir, after);
    }

    b->lfunc->block = after;
}
