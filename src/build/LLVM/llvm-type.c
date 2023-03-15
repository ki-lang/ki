
#include "../../headers/LLVM.h"

char *llvm_type(LB *b, Type *type) {
    //
    Class *class = type->class;
    if (type_is_void(type)) {
        return "void";
    }
    if (type->type == type_null) {
        return "i8*";
    }
    if (class->type == ct_struct) {
        char name[100];
        sprintf(name, "%%struct.%s", class->gname);
        if (!array_contains(b->defined_classes, class, arr_find_adr)) {
            array_push(b->defined_classes, class);

            Str *ir = str_make(b->alc, 1000);
            str_append_chars(ir, name);
            str_append_chars(ir, " = type { ");

            for (int i = 0; i < class->props->keys->length; i++) {
                ClassProp *prop = array_get_index(class->props->values, i);
                if (i > 0) {
                    str_append_chars(ir, ", ");
                }
                str_append_chars(ir, llvm_type(b, prop->type));
            }
            str_append_chars(ir, " }\n");

            str_append(b->ir_struct, ir);
        }
        char *res = al(b->alc, strlen(name) + 2);
        strcpy(res, name);
        strcat(res, "*");
        return res;
    }

    if (type->type == type_func_ptr) {
        Str *res = str_make(b->alc, 1000);
        Type *rett = type->func_rett;
        char *lrett = "void";
        if (rett) {
            lrett = llvm_type(b, rett);
        }
        str_append_chars(res, lrett);
        str_append_chars(res, "(");
        Array *args = type->func_args;
        for (int i = 0; i < args->length; i++) {
            Arg *arg = array_get_index(args, i);
            if (i > 0) {
                str_append_chars(res, ", ");
            }
            str_append_chars(res, llvm_type(b, arg->type));
        }
        str_append_chars(res, ")*");
        return str_to_chars(b->alc, res);
    }
    if (type->ptr_depth > 0) {
        return "i8*";
    }
    int bytes = type->bytes;
    return llvm_type_int(b, bytes);
}

char *llvm_type_int(LB *b, int bytes) {
    if (bytes == 1) {
        return "i8";
    } else if (bytes == 2) {
        return "i16";
    } else if (bytes == 4) {
        return "i32";
    } else if (bytes == 8) {
        return "i64";
    }

    sprintf(b->fc->sbuf, "LLVM: Unsupported type size: '%d'", bytes);
    die(b->fc->sbuf);
    return "";
}

char *llvm_type_ixx(LB *b) {
    //
    return llvm_type_int(b, b->fc->b->ptr_size);
}
