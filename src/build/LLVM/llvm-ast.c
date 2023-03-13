
#include "../../headers/LLVM.h"

void llvm_if(LB *b, Scope *scope, TIf *ift, bool is_else, LLVMBlock *after);

void llvm_write_ast(LB *b, Scope *scope) {
    //
    if (!scope->lvars)
        scope->lvars = map_make(b->alc);

    Allocator *alc = b->alc;

    Array *ast = scope->ast;
    for (int i = 0; i < ast->length; i++) {
        Token *t = array_get_index(ast, i);

        if (t->type == tkn_declare) {
            Decl *decl = t->item;
            Value *val = decl->value;

            char *lval = llvm_value(b, scope, val);

            if (!decl->is_mut) {
                map_set(scope->lvars, decl->name, lval);
                continue;
            }

            char *lvar = llvm_alloca(b, decl->type);
            llvm_ir_store(b, decl->type, lvar, lval);
            map_set(scope->lvars, decl->name, lvar);
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
        if (t->type == tkn_upref_slot) {
            UprefSlot *up = t->item;
            Decl *decl = up->decl;
            if (decl->times_used > 1 && up->count > 0) {
                // printf("UP:%d | %d\n", up->count, decl->times_used);
                Type *type = decl->type;

                Value *var = value_init(alc, v_var, var_init(alc, decl, type), type);
                Scope *sub = scope_init(alc, sct_default, scope, true);
                class_ref_change(alc, sub, var, -1);

                llvm_write_ast(b, sub);
            }
            continue;
        }
        if (t->type == tkn_deref) {
            Value *val = t->item;
            Scope *sub = scope_init(alc, sct_default, scope, true);
            class_ref_change(alc, sub, val, -1);
            llvm_write_ast(b, sub);
            continue;
        }
        if (t->type == tkn_tmp_var) {
            TempVar *tmp = t->item;
            tmp->ir_value = llvm_value(b, scope, tmp->value);
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
        if (t->type == tkn_if) {
            TIf *ift = t->item;
            llvm_if(b, scope, ift, false, NULL);
            continue;
        }
        if (t->type == tkn_while) {
            TWhile *w = t->item;

            Value *cond = w->cond;
            Scope *sub = w->scope;

            LLVMBlock *b_cond = llvm_block_init_auto(b);
            LLVMBlock *b_code = llvm_block_init_auto(b);
            LLVMBlock *b_after = llvm_block_init_auto(b);
            LLVMBlock *prev_cond = b->while_cond;
            LLVMBlock *prev_after = b->while_after;
            b->while_cond = b_cond;
            b->while_after = b_after;

            llvm_ir_jump(llvm_b_ir(b), b_cond);

            b->lfunc->block = b_cond;
            char *lcond = llvm_value(b, scope, cond);
            Str *ir_c = llvm_b_ir(b);
            char *lcond_i1 = llvm_ir_bool_i1(b, ir_c, lcond);
            llvm_ir_cond_jump(b, ir_c, lcond_i1, b_code, b_after);

            b->lfunc->block = b_code;
            llvm_write_ast(b, sub);
            if (!sub->did_return) {
                llvm_ir_jump(llvm_b_ir(b), b_cond);
            }

            b->lfunc->block = b_after;
            b->while_cond = prev_cond;
            b->while_after = prev_after;
            continue;
        }
        if (t->type == tkn_statement) {
            Value *val = t->item;
            char *lval = llvm_value(b, scope, val);
            continue;
        }
    }
}

void llvm_if(LB *b, Scope *scope, TIf *ift, bool is_else, LLVMBlock *after) {
    //
    Value *cond = ift->cond;
    Scope *sub = ift->scope;
    TIf *elif = ift->else_if;

    if (!cond) {
        llvm_write_ast(b, sub);
        if (!sub->did_return) {
            Str *ir = llvm_b_ir(b);
            llvm_ir_jump(ir, after);
        }
        return;
    }

    LLVMBlock *code_block = llvm_block_init_auto(b);
    LLVMBlock *else_block = NULL;

    if (elif) {
        else_block = llvm_block_init_auto(b);
    }
    if (after == NULL) {
        after = llvm_block_init_auto(b);
    }

    char *lcond = llvm_value(b, scope, cond);
    Str *ir = llvm_b_ir(b);

    char *lcond_i1 = llvm_ir_bool_i1(b, ir, lcond);
    if (else_block) {
        llvm_ir_cond_jump(b, ir, lcond_i1, code_block, else_block);
    } else {
        llvm_ir_cond_jump(b, ir, lcond_i1, code_block, after);
    }

    b->lfunc->block = code_block;
    llvm_write_ast(b, sub);
    if (!sub->did_return) {
        Str *ir = llvm_b_ir(b);
        llvm_ir_jump(ir, after);
    }

    if (elif) {
        b->lfunc->block = else_block;
        llvm_if(b, scope, elif, true, after);
    }

    if (!is_else) {
        b->lfunc->block = after;
    }
}
