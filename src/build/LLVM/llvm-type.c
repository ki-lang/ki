
#include "../../headers/LLVM.h"

void llvm_check_defined(LB *b, Class *class) {
    //
    if (!array_contains(b->defined_classes, class, arr_find_adr)) {
        char name[100];
        sprintf(name, "%%struct.%s", class->gname);

        array_push(b->defined_classes, class);

        Str *ir = str_make(b->alc, 1000);
        str_append_chars(ir, name);
        str_append_chars(ir, " = type ");
        str_append_chars(ir, class->packed ? "<{ " : "{ ");

        for (int i = 0; i < class->props->keys->length; i++) {
            ClassProp *prop = array_get_index(class->props->values, i);
            if (i > 0) {
                str_append_chars(ir, ", ");
            }
            str_append_chars(ir, llvm_type(b, prop->type));
        }
        str_append_chars(ir, class->packed ? " }>\n" : " }\n");

        str_append(b->ir_struct, ir);
    }
}

char *llvm_type_real(LB *b, Type *type) {
    Class *class = type->class;

    if (class && class->type == ct_struct) {
        Str *result = str_make(b->alc, 100);
        int depth = type->ptr_depth;

        llvm_check_defined(b, class);

        char name[100];
        sprintf(name, "%%struct.%s", class->gname);
        str_append_chars(result, name);
        //
        while (depth > 0) {
            str_append_chars(result, "*");
            depth--;
        }
        //
        return str_to_chars(b->alc, result);
    }

    return llvm_type(b, type);
}

char *llvm_type(LB *b, Type *type) {
    //
    int depth = type->ptr_depth;
    if (depth > 0) {
        return "ptr";
    }

    Str *result = str_make(b->alc, 50);
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

        llvm_check_defined(b, class);

        char name[100];
        sprintf(name, "%%struct.%s", class->gname);
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
    return str_to_chars(b->alc, result);
}

char *llvm_di_type(LB *b, Type *type) {
    //
    if (type_is_void(type)) {
        return "null";
    }

    // Number
    // !13 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)

    // Pointer
    // !13 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)

    // Struct
    // !13 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !14, size: 64)
    // !14 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "A", file: !1, line: 12, size: 128, elements: !15)
    // !15 = !{!16, !18}
    // !16 = !DIDerivedType(tag: DW_TAG_member, name: "v", scope: !14, file: !1, line: 13, baseType: !17, size: 32)
    // !17 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
    // !18 = !DIDerivedType(tag: DW_TAG_member, name: "b", scope: !14, file: !1, line: 14, baseType: !19, size: 64, offset: 64)
    // !19 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !20, size: 64)
    // !20 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "B", file: !1, line: 16, size: 32, elements: !21)
    // !21 = !{!22}
    // !22 = !DIDerivedType(tag: DW_TAG_member, name: "v", scope: !20, file: !1, line: 17, baseType: !17, size: 32)

    // Function pointer
    // !23 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !24, size: 64)
    // !24 = !DISubroutineType(types: !25)
    // !25 = !{!17, !26, !26}
    // !26 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !17)

    if (type->ptr_depth > 0) {
        if (!b->di_type_ptr) {
            char size[10];
            sprintf(size, "%d", b->fc->b->ptr_size * 8);
            char *attr = llvm_attr(b);
            Str *buf = str_make(b->alc, 80);
            str_append_chars(buf, attr);
            str_append_chars(buf, " = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: ");
            str_append_chars(buf, size);
            str_append_chars(buf, ")");
            array_push(b->attrs, str_to_chars(b->alc, buf));
            b->di_type_ptr = attr;
        }
        return b->di_type_ptr;
    }

    Class *class = type->class;
    if (class) {
        // if (class->di_attr) {
        //     return class->di_attr;
        // }
        char *attr = llvm_attr(b);
        Str *buf = str_make(b->alc, 80);
        str_append_chars(buf, attr);
        str_append_chars(buf, " = ");
        char size[10];

        if (class->type == ct_int) {
            sprintf(size, "%d", type->bytes * 8);
            str_append_chars(buf, "!DIBasicType(name: \"");
            str_append_chars(buf, "\", size: ");
            str_append_chars(buf, size);
            str_append_chars(buf, ", encoding: ");
            str_append_chars(buf, class->is_signed ? "DW_ATE_signed" : "DW_ATE_unsigned");
            str_append_chars(buf, ")");

            array_push(b->attrs, str_to_chars(b->alc, buf));
            // class->di_attr = attr;
            return attr;
        }
        if (class->type == ct_float) {
            sprintf(size, "%d", type->bytes * 8);
            str_append_chars(buf, "!DIBasicType(name: \"");
            str_append_chars(buf, "\", size: ");
            str_append_chars(buf, size);
            str_append_chars(buf, ", encoding: DW_ATE_float)");

            array_push(b->attrs, str_to_chars(b->alc, buf));
            // class->di_attr = attr;
            return attr;
        }
    }

    sprintf(b->fc->sbuf, "LLVM: Cannot generate debug type");
    die(b->fc->sbuf);
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
