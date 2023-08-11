
#include "../../headers/LLVM.h"

char *llvm_ir_isnull_i1(LB *b, char *ltype, char *val) {
    Str *ir = llvm_b_ir(b);
    char *var_i1 = llvm_var(b);
    str_append_chars(ir, "  ");
    str_append_chars(ir, var_i1);
    str_append_chars(ir, " = icmp eq ");
    str_append_chars(ir, ltype);
    str_append_chars(ir, " ");
    str_append_chars(ir, val);
    str_append_chars(ir, ", null\n");
    return var_i1;
}
char *llvm_ir_notnull_i1(LB *b, char *ltype, char *val) {
    Str *ir = llvm_b_ir(b);
    char *var_i1 = llvm_var(b);
    str_append_chars(ir, "  ");
    str_append_chars(ir, var_i1);
    str_append_chars(ir, " = icmp ne ");
    str_append_chars(ir, ltype);
    str_append_chars(ir, " ");
    str_append_chars(ir, val);
    str_append_chars(ir, ", null\n");
    return var_i1;
}

char *llvm_ir_iszero_i1(LB *b, char *ltype, char *val) {
    Str *ir = llvm_b_ir(b);
    char *var_i1 = llvm_var(b);
    str_append_chars(ir, "  ");
    str_append_chars(ir, var_i1);
    str_append_chars(ir, " = icmp eq ");
    str_append_chars(ir, ltype);
    str_append_chars(ir, " ");
    str_append_chars(ir, val);
    str_append_chars(ir, ", 0\n");
    return var_i1;
}

char *llvm_ir_istrue_i1(LB *b, char *val) {
    Str *ir = llvm_b_ir(b);
    char *var_i1 = llvm_var(b);
    str_append_chars(ir, "  ");
    str_append_chars(ir, var_i1);
    str_append_chars(ir, " = icmp eq i8 ");
    str_append_chars(ir, val);
    str_append_chars(ir, ", 1\n");
    return var_i1;
}

char *llvm_ir_cmp(LB *b, char *ltype, char *val, char *cmd, char *with) {
    // cmd: eq,ne
    Str *ir = llvm_b_ir(b);
    char *var_i1 = llvm_var(b);
    str_append_chars(ir, "  ");
    str_append_chars(ir, var_i1);
    str_append_chars(ir, " = icmp ");
    str_append_chars(ir, cmd);
    str_append_chars(ir, " ");
    str_append_chars(ir, ltype);
    str_append_chars(ir, " ");
    str_append_chars(ir, val);
    str_append_chars(ir, ", ");
    str_append_chars(ir, with);
    str_append_chars(ir, "\n");
    return var_i1;
}

void llvm_ir_jump(Str *ir, LLVMBlock *block) {
    str_append_chars(ir, "  br label %");
    str_append_chars(ir, block->name);
    str_append_chars(ir, "\n");
}
void llvm_ir_jump_loop(LB *b, LLVMBlock *block) {
    Str *ir = llvm_b_ir(b);
    str_append_chars(ir, "  br label %");
    str_append_chars(ir, block->name);
    str_append_chars(ir, ", !llvm.loop ");
    str_append_chars(ir, b->loop_attr);
    str_append_chars(ir, "\n");
}

void llvm_ir_cond_jump(LB *b, Str *ir, char *var_i1, LLVMBlock *a_block, LLVMBlock *b_block) {
    str_append_chars(ir, "  br i1 ");
    str_append_chars(ir, var_i1);
    str_append_chars(ir, ", label %");
    str_append_chars(ir, a_block->name);
    str_append_chars(ir, ", label %");
    str_append_chars(ir, b_block->name);
    str_append_chars(ir, "\n");
}

void llvm_ir_store(LB *b, Type *type, char *var, char *val) {
    Str *ir = llvm_b_ir(b);
    char *ltype = llvm_type(b, type);

    char bytes[20];
    int abytes = type->bytes;
    if (abytes > b->fc->b->ptr_size) {
        abytes = b->fc->b->ptr_size;
    }
    sprintf(bytes, "%d", abytes);

    str_append_chars(ir, "  store ");
    str_append_chars(ir, ltype);
    str_append_chars(ir, " ");
    str_append_chars(ir, val);
    str_append_chars(ir, ", ptr ");
    str_append_chars(ir, var);
    str_append_chars(ir, ", align ");
    str_append_chars(ir, bytes);
    str_append_chars(ir, "\n");
}
char *llvm_ir_load(LB *b, Type *type, char *var) {

    Str *ir = llvm_b_ir(b);
    char *var_result = llvm_var(b);
    char *ltype = llvm_type(b, type);

    char bytes[20];
    int abytes = type->bytes;
    if (abytes > b->fc->b->ptr_size) {
        abytes = b->fc->b->ptr_size;
    }
    sprintf(bytes, "%d", abytes);

    str_append_chars(ir, "  ");
    str_append_chars(ir, var_result);
    str_append_chars(ir, " = load ");
    str_append_chars(ir, ltype);
    str_append_chars(ir, ", ptr ");
    str_append_chars(ir, var);
    str_append_chars(ir, ", align ");
    str_append_chars(ir, bytes);
    str_append_chars(ir, "\n");

    return var_result;
}

char *llvm_ir_bool_i1(LB *b, Str *ir, char *val) {
    char *var_i1 = llvm_var(b);
    str_append_chars(ir, "  ");
    str_append_chars(ir, var_i1);
    str_append_chars(ir, " = trunc i8 ");
    str_append_chars(ir, val);
    str_append_chars(ir, " to i1\n");
    return var_i1;
}

char *llvm_ir_class_prop_access(LB *b, Class *class, char *on, ClassProp *prop) {
    char *result = llvm_var(b);
    Str *ir = llvm_b_ir(b);

    char index[20];
    sprintf(index, "%d", prop->index);

    llvm_check_defined(b, class);

    str_append_chars(ir, "  ");
    str_append_chars(ir, result);
    str_append_chars(ir, " = getelementptr inbounds %struct.");
    str_append_chars(ir, class->gname);
    str_append_chars(ir, ", ptr ");
    str_append_chars(ir, on);
    str_append_chars(ir, ", i32 0, i32 ");
    str_append_chars(ir, index);
    str_append_chars(ir, "\n");

    return result;
}

Array *llvm_ir_fcall_args(LB *b, Scope *scope, Array *values) {
    Array *result = array_make(b->alc, values->length + 1);
    for (int i = 0; i < values->length; i++) {
        Value *val = array_get_index(values, i);
        char *lval = llvm_value(b, scope, val);
        char *buf = b->fc->sbuf;
        sprintf(buf, "%s noundef%s %s", llvm_type(b, val->rett), val->rett->ptr_depth == 0 || val->rett->nullable ? "" : " nonnull", lval);
        array_push(result, dups(b->alc, buf));
    }
    return result;
}
char *llvm_ir_fcall_arg(LB *b, char *lval, char *ltype) {
    char *buf = b->fc->sbuf;
    sprintf(buf, "%s noundef %s", ltype, lval);
    return dups(b->alc, buf);
}

char *llvm_ir_func_call(LB *b, char *on, Array *values, char *lrett, FCallOr *ort, int line, int col) {
    Str *ir = llvm_b_ir(b);
    // if (can_error) {
    // die("TODO LLVM Func error");
    // TODO dont reset err before each call, reset when err is caught instead
    // llvm_ir_store(b, type_gen(b->fc->b, "i32"), b->func_buf_err, "0");
    // }

    char *var_result = "";
    str_append_chars(ir, "  ");
    if (strcmp(lrett, "void") != 0) {
        var_result = llvm_var(b);
        str_append_chars(ir, var_result);
        str_append_chars(ir, " = ");
    }
    str_append_chars(ir, "call ");
    str_append_chars(ir, lrett);
    str_append_chars(ir, " ");
    str_append_chars(ir, on);
    str_append_chars(ir, "(");
    int argc = values->length;
    for (int i = 0; i < values->length; i++) {
        char *lval = array_get_index(values, i);
        if (i > 0) {
            str_append_chars(ir, ", ");
        }
        str_append_chars(ir, lval);
    }
    str_append_chars(ir, ")");
    if (b->debug) {
        char *loc = llvm_attr(b);
        str_append_chars(ir, ", !dbg ");
        str_append_chars(ir, loc);
        sprintf(b->char_buf, "%s = !DILocation(line: %d, column: %d, scope: %s)", loc, line, col, b->lfunc->di_scope);
        array_push(b->attrs, dups(b->alc, b->char_buf));
    }
    str_append_chars(ir, "\n");

    if (ort) {

        Type *err_code_type = type_gen(b->fc->b, b->alc, "i32");
        Type *err_msg_type = type_gen(b->fc->b, b->alc, "String");

        char *load_err = llvm_ir_load(b, err_code_type, "@ki_err_code_buffer");
        char *iszero = llvm_ir_iszero_i1(b, "i32", load_err);
        if (ort->err_code_decl) {
            Decl *err_decl = ort->err_code_decl;
            err_decl->llvm_val = llvm_ir_load(b, err_code_type, "@ki_err_code_buffer");
        }
        if (ort->err_msg_decl) {
            Decl *err_decl = ort->err_msg_decl;
            err_decl->llvm_val = llvm_ir_load(b, err_msg_type, "@ki_err_msg_buffer");
        }

        if (ort->value) {
            // !?
            LLVMBlock *b_code = llvm_block_init_auto(b);
            LLVMBlock *b_else = llvm_block_init_auto(b);
            LLVMBlock *b_after = llvm_block_init_auto(b);

            char *current_block_name = b->lfunc->block->name;

            if (ort->deref_scope)
                llvm_write_ast(b, ort->deref_scope);

            llvm_ir_cond_jump(b, llvm_b_ir(b), iszero, b_else, b_code);

            b->lfunc->block = b_code;
            llvm_ir_store(b, err_code_type, "@ki_err_code_buffer", "0");
            llvm_write_ast(b, ort->scope);
            char *rlval = llvm_value(b, ort->scope, ort->value);
            llvm_ir_jump(llvm_b_ir(b), b_after);

            char *last_block_code = b->lfunc->block->name;

            b->lfunc->block = b_else;
            llvm_write_ast(b, ort->else_scope);
            llvm_ir_jump(llvm_b_ir(b), b_after);

            char *last_block_else = b->lfunc->block->name;

            b->lfunc->block = b_after;
            Str *ir = b_after->ir;

            char *new_result = llvm_var(b);

            str_append_chars(ir, "  ");
            str_append_chars(ir, new_result);
            str_append_chars(ir, " = phi ");
            str_append_chars(ir, lrett);
            str_append_chars(ir, " [ ");
            str_append_chars(ir, var_result);
            str_append_chars(ir, ", %");
            str_append_chars(ir, last_block_else);
            str_append_chars(ir, " ], [ ");
            str_append_chars(ir, rlval);
            str_append_chars(ir, ", %");
            str_append_chars(ir, last_block_code);
            str_append_chars(ir, " ]\n");

            var_result = new_result;

        } else {
            // !!
            LLVMBlock *b_code = llvm_block_init_auto(b);
            LLVMBlock *b_else = llvm_block_init_auto(b);
            LLVMBlock *b_after = llvm_block_init_auto(b);

            if (ort->deref_scope)
                llvm_write_ast(b, ort->deref_scope);

            llvm_ir_cond_jump(b, llvm_b_ir(b), iszero, b_else, b_code);

            b->lfunc->block = b_code;
            llvm_ir_store(b, err_code_type, "@ki_err_code_buffer", "0");
            llvm_write_ast(b, ort->scope);
            if (!ort->scope->did_return) {
                llvm_ir_jump(llvm_b_ir(b), b_after);
            }

            b->lfunc->block = b_else;
            llvm_write_ast(b, ort->else_scope);
            if (!ort->else_scope->did_return) {
                llvm_ir_jump(llvm_b_ir(b), b_after);
            }

            b->lfunc->block = b_after;
        }
    }

    return var_result;
}

char *llvm_ir_func_ptr(LB *b, Func *func) {
    //
    if (func->fc != b->fc) {
        // Extern function
        llvm_define_ext_func(b, func);
    }
    char buf[strlen(func->gname) + 2];
    strcpy(buf, "@");
    strcat(buf, func->gname);
    return dups(b->alc, buf);
}

char *llvm_ir_cast(LB *b, char *lval, Type *from_type, Type *to_type) {
    //
    Str *ir = llvm_b_ir(b);

    char *lfrom_type = llvm_type(b, from_type);
    char *lto_type = llvm_type(b, to_type);
    char *result_var = lval;

    if (from_type->ptr_depth > 0 && to_type->ptr_depth == 0) {
        // Ptr to int
        char *var = llvm_var(b);
        str_append_chars(ir, "  ");
        str_append_chars(ir, var);
        str_append_chars(ir, " = ptrtoint ");
        str_append_chars(ir, lfrom_type);
        str_append_chars(ir, " ");
        str_append_chars(ir, result_var);
        str_append_chars(ir, " to ");
        str_append_chars(ir, lto_type);
        str_append_chars(ir, "\n");
        result_var = var;
    } else if (from_type->ptr_depth == 0) {
        if (from_type->bytes < to_type->bytes) {
            // Ext
            char *new_type = llvm_type_int(b, to_type->bytes);
            char *var = llvm_var(b);
            str_append_chars(ir, "  ");
            str_append_chars(ir, var);
            if (from_type->is_signed) {
                str_append_chars(ir, " = sext ");
            } else {
                str_append_chars(ir, " = zext ");
            }
            str_append_chars(ir, lfrom_type);
            str_append_chars(ir, " ");
            str_append_chars(ir, result_var);
            str_append_chars(ir, " to ");
            str_append_chars(ir, new_type);
            str_append_chars(ir, "\n");
            lfrom_type = new_type;
            result_var = var;
        } else if (from_type->bytes > to_type->bytes) {
            // Trunc
            char *new_type = llvm_type_int(b, to_type->bytes);
            char *var = llvm_var(b);
            str_append_chars(ir, "  ");
            str_append_chars(ir, var);
            str_append_chars(ir, " = trunc ");
            str_append_chars(ir, lfrom_type);
            str_append_chars(ir, " ");
            str_append_chars(ir, result_var);
            str_append_chars(ir, " to ");
            str_append_chars(ir, new_type);
            str_append_chars(ir, "\n");
            lfrom_type = new_type;
            result_var = var;
        }
        if (to_type->ptr_depth > 0) {
            // Bitcast to i8*|%struct...*
            char *var = llvm_var(b);
            str_append_chars(ir, "  ");
            str_append_chars(ir, var);
            str_append_chars(ir, " = inttoptr ");
            str_append_chars(ir, lfrom_type);
            str_append_chars(ir, " ");
            str_append_chars(ir, result_var);
            str_append_chars(ir, " to ptr\n");
            result_var = var;
        }
    }

    return result_var;
}

char *llvm_ir_string(LB *b, char *body) {
    //
    Fc *fc = b->fc;

    b->strc++;
    Str *ir = b->ir_global;

    sprintf(fc->sbuf, "@.str.%d", b->strc);
    char *var = dups(b->alc, fc->sbuf);

    int ptr_size = b->fc->b->ptr_size;
    int len = strlen(body);
    int blen = len + ptr_size + 1;

    sprintf(fc->sbuf, "%s = private unnamed_addr constant [%d x i8] c\"", var, blen);
    str_append_chars(ir, fc->sbuf);

    // Bytes
    // Len bytes
    size_t len_buf = len;
    unsigned char *len_ptr = (unsigned char *)&len_buf;
    int c = 0;
    while (c < ptr_size) {
        unsigned char ch = *(len_ptr + c);
        c++;
        str_append_char(ir, '\\');
        char hex[20];
        sprintf(hex, "%02X", ch);
        if (strlen(hex) == 0) {
            str_append_char(ir, '0');
        }
        str_append_chars(ir, hex);
    }

    // Const byte
    str_append_chars(ir, "\\01");

    // String bytes
    int index = 0;
    while (index < len) {
        unsigned char ch = body[index];
        index++;
        if (ch > 34 && ch < 127 && ch != 92) {
            str_append_char(ir, ch);
            continue;
        }
        str_append_char(ir, '\\');
        char hex[20];
        sprintf(hex, "%02X", ch);
        if (strlen(hex) == 0) {
            str_append_char(ir, '0');
        }
        str_append_chars(ir, hex);
    }
    //
    str_append_chars(ir, "\", align 8\n");

    sprintf(fc->sbuf, "getelementptr inbounds ([%d x i8], [%d x i8]* %s, i64 0, i64 0)", blen, blen, var);
    return dups(b->alc, fc->sbuf);
}

char *llvm_ir_stack_alloc(LB *b, char *amount, char *amount_type) {
    //
    Str *ir = llvm_b_ir(b);

    char *var = llvm_var(b);
    str_append_chars(ir, "  ");
    str_append_chars(ir, var);
    str_append_chars(ir, " = alloca i8, ");
    str_append_chars(ir, amount_type);
    str_append_chars(ir, " ");
    str_append_chars(ir, amount);
    str_append_chars(ir, ", align 8\n");

    return var;
}
char *llvm_ir_gep(LB *b, char *type, char *lon, char *index, char *index_type) {
    //
    Str *ir = llvm_b_ir(b);
    char *result = llvm_var(b);
    str_append_chars(ir, "  ");
    str_append_chars(ir, result);
    str_append_chars(ir, " = getelementptr inbounds ");
    str_append_chars(ir, type);
    str_append_chars(ir, ", ptr ");
    str_append_chars(ir, lon);
    str_append_chars(ir, ", ");
    str_append_chars(ir, index_type);
    str_append_chars(ir, " ");
    str_append_chars(ir, index);
    str_append_chars(ir, "\n");

    return result;
}

char *llvm_ir_atomic_xchange(LB *b, char *var, char *type, char *new_value) {
    Str *ir = llvm_b_ir(b);
    char *result = llvm_var(b);
    str_append_chars(ir, "  ");
    str_append_chars(ir, var);
    str_append_chars(ir, " = atomicrmw xchg ");
    str_append_chars(ir, type);
    str_append_chars(ir, "* ");
    str_append_chars(ir, var);
    str_append_chars(ir, ", ");
    str_append_chars(ir, type);
    str_append_chars(ir, " ");
    str_append_chars(ir, new_value);
    str_append_chars(ir, " seq_cst\n");
    return result;
}
