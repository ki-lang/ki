
#include "../../headers/LLVM.h"

char *llvm_type(LB *b, Type *type) {
    //
    Str *result = str_make(b->alc, 100);
    int depth = type->ptr_depth;
    Class *class = type->class;

    if (type_is_void(type)) {
        str_append_chars(result, "void");
        //
    } else if (type->type == type_null) {
        str_append_chars(result, "i8");
        //
    } else if (type->type == type_ptr) {
        str_append_chars(result, "i8");
        //
    } else if (class && class->type == ct_struct) {
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
        str_append_chars(result, name);
        //
    } else if (type->type == type_func_ptr) {
        Type *rett = type->func_rett;
        char *lrett = "void";
        if (rett) {
            lrett = llvm_type(b, rett);
        }
        str_append_chars(result, lrett);
        str_append_chars(result, "(");
        Array *args = type->func_args;
        for (int i = 0; i < args->length; i++) {
            Arg *arg = array_get_index(args, i);
            if (i > 0) {
                str_append_chars(result, ", ");
            }
            str_append_chars(result, llvm_type(b, arg->type));
        }
        str_append_chars(result, ")");
        //
    } else if (type->type == type_int) {
        int bytes = type->bytes;
        char *num_type = llvm_type_int(b, bytes);
        str_append_chars(result, num_type);
    } else if (type->type == type_arr) {
        char *lof = llvm_type(b, type->array_of);
        str_append_chars(result, lof);
    } else {
        printf("Type: %d\n", type->type);
        die("Unknown LLVM type (bug)");
    }
    //
    while (depth > 0) {
        str_append_chars(result, "*");
        depth--;
    }
    //
    return str_to_chars(b->alc, result);
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
