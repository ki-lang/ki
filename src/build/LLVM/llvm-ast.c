
#include "../../headers/LLVM.h"

char *llvm_write_ast(LB *b, Scope *scope) {
    //
    if (!scope->lvars)
        scope->lvars = map_make(b->alc);

    // die("AST TODO");
    Array *ast = scope->ast;
    for (int i = 0; i < ast->length; i++) {
        Token *t = array_get_index(ast, i);

        if (t->type == tkn_declare) {
            TDecl *decl = t->item;
            Var *var = decl->var;
            Value *val = decl->value;

            char *lval = llvm_value(b, scope, val);

            if (!var->is_mut) {
                map_set(scope->lvars, var->name, lval);
                continue;
            }

            char *lvar = llvm_alloca(b, var->type);
            llvm_ir_store(b, var->type, lvar, lval);
            map_set(scope->lvars, var->name, lvar);
            continue;
        }
        if (t->type == tkn_assign) {
            VPair *pair = t->item;
            char *lvar = llvm_assign_value(b, scope, pair->left);
            char *lval = llvm_value(b, scope, pair->right);
            llvm_ir_store(b, pair->left->rett, lvar, lval);
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
        }
        if (t->type == tkn_statement) {
            Value *val = t->item;
            char *lval = llvm_value(b, scope, val);
        }
    }
}
