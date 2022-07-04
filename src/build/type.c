
#include "../all.h"
#include "../libs/md5.h"

Type *init_type() {
    Type *type = malloc(sizeof(Type));
    type->type = type_unknown;
    type->nullable = false;
    type->npt = false;
    type->allow_math = false;
    type->is_float = false;
    type->is_unsigned = false;
    type->is_pointer = false;
    type->is_pointer_of_pointer = false;
    type->is_array = false;
    //
    type->c_inline = false;
    type->c_static = false;
    //
    type->array_size = 0;
    type->bytes = 0;
    type->class = NULL;
    type->enu = NULL;
    //
    type->func_arg_types = NULL;
    type->func_return_type = NULL;
    type->func_can_error = false;
    return type;
}

void free_type(Type *type) {
    //
    free(type);
}

Type *fc_read_type(FileCompiler *fc, Scope *scope) {
    //
    char *token = malloc(KI_TOKEN_MAX);
    bool nullable = false;
    bool npt = false;
    //
    fc_next_token(fc, token, false, false, true);
    if (strcmp(token, "?") == 0) {
        nullable = true;
        fc_next_token(fc, token, false, false, true);
    }

    if (strcmp(token, "NPT") == 0) {
        npt = true;
        fc_next_token(fc, token, false, false, true);
        if (strcmp(token, "?") == 0) {
            nullable = true;
        }
    }

    if (npt && nullable) {
        fc_error(fc, "NPT (non pointer type) cannot be NULL", NULL);
    }

    //
    if (!is_valid_varname(token)) {
        fc_error(fc, "Invalid type syntax: '%s'", token);
    }

    //
    fc->i -= strlen(token);
    Identifier *id = fc_read_identifier(fc, false, true, false);
    Type *t = fc_identifier_to_type(fc, id, scope);
    // if (t && t->class) {
    //     printf("%s\n", t->class->cname);
    // }
    if (t == NULL) {
        fc_error(fc, "Unknown type/class/enum: '%s'", token);
    }

    // enum
    if (t->class) {
        if (t->class->generic_names != NULL && t->class->generic_hash == NULL) {
            // Generic class
            Class *gclass = fc_get_generic_class(fc, t->class, scope);
            t->class = gclass;
        }

        fc_depends_on(fc, t->class->fc);

        if (fc_get_char(fc, 0) == '.') {
            // Enum type
            fc->i++;
            fc_next_token(fc, token, false, true, false);
            Class *class = t->class;
            ClassProp *prop = map_get(class->props, token);
            if (!prop) {
                fc_error(fc, "Property '%s' not found on this class", token);
            }
            if (prop->return_type->type != type_enum) {
                fc_error(fc, "Property '%s' cannot be used as a type", token);
            }
            free(t);
            t = prop->return_type;
        }
    }

    //
    if (nullable) {
        fc_type_make_nullable(fc, t);
    }
    t->npt = npt;

    if (t->npt && t->type != type_struct) {
        fc_error(fc, "NPT (not pointer type) can only be applied to class instances", NULL);
    }

    if (t->npt) {
        t->is_pointer = false;
    }

    // Check if array
    fc_next_token(fc, token, true, true, false);
    if (strcmp(token, "[") == 0) {
        t->is_array = true;
        fc_next_token(fc, token, false, true, false);
        fc_next_token(fc, token, false, true, true);
        if (!is_valid_number(token)) {
            fc_error(fc, "Invalid number: '%s'", token);
        }
        int size = atoi(token);
        t->array_size = size;
        fc_expect_token(fc, "]", false, true, true);
    }

    free_id(id);
    free(token);
    return t;
}

void fc_type_make_nullable(FileCompiler *fc, Type *t) {
    //
    if (!t->is_pointer) {
        // // Generate ?Value<T> type
        // Type *subtype = init_type();
        // *subtype = *t; // Copy values

        // Type *opt_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "Value"), NULL);

        // Array *subtypes = array_make(2);
        // array_push(subtypes, subtype);
        // Class *gclass = fc_get_generic_class_by_hash(opt_type->class, subtypes);
        // // array_free(subtypes); // shouldnt free values

        // t->type = type_struct;
        // t->class = gclass;
        // t->is_pointer = true;
        // t->bytes = pointer_size;
        // t->allow_math = false;
        //
        fc_error(fc, "Invalid type, only pointer types can be null", NULL);
    }
    t->nullable = true;
}

Type *fc_identifier_to_type(FileCompiler *fc, Identifier *id, Scope *scope) {
    Type *t = init_type();

    PkgCompiler *pkc = fc->nsc->pkc;
    if (id->package != NULL) {
        pkc = pkc_get_by_name(id->package);
    }
    NsCompiler *nsc = fc->nsc;
    if (id->namespace != NULL) {
        nsc = pkc_get_namespace_by_name(pkc, id->namespace);
        scope = nsc->scope;
    }

    // Read standard type
    if (id->namespace == NULL) {
        if (strcmp(id->name, "void") == 0) {
            t->type = type_void;
        } else if (strcmp(id->name, "funcref") == 0) {
            char *token = malloc(KI_TOKEN_MAX);
            t->type = type_funcref;
            fc_expect_token(fc, "(", false, true, true);
            // Read arg types
            t->func_arg_types = array_make(4);
            fc_next_token(fc, token, true, true, true);
            while (strcmp(token, ")") != 0) {
                Type *arg_type = fc_read_type(fc, scope);
                array_push(t->func_arg_types, arg_type);
                fc_next_token(fc, token, true, true, true);
                if (strcmp(token, ",") == 0) {
                    fc_next_token(fc, token, false, true, true);
                    fc_next_token(fc, token, true, true, true);
                } else {
                    fc_expect_token(fc, ")", false, true, true);
                }
            }

            // Read return type
            fc_next_token(fc, token, true, true, true);
            if (strcmp(token, "!") == 0) {
                t->func_can_error = true;
                fc_next_token(fc, token, false, true, true);
            }
            t->func_return_type = fc_read_type(fc, scope);
            t->is_pointer = true;
            t->bytes = pointer_size;

            free(token);
        }
    }

    if (id->package && strcmp(id->package, "ki") == 0 && strcmp(id->namespace, "type") == 0) {
        if (strcmp(id->name, "ptr") == 0) {
            t->type = type_void_pointer;
            IdentifierFor *idf = idf_find_in_scope(scope, id);
            if (idf == NULL) {
                fc_error(fc, "Compiler bug, cant find ptr class", NULL);
            }
            t->is_pointer = true;
            t->class = idf->item;
            t->bytes = pointer_size;
            t->allow_math = true;
            t->nullable = true;
        } else if (strcmp(id->name, "bool") == 0) {
            t->type = type_bool;
            IdentifierFor *idf = idf_find_in_scope(scope, id);
            if (idf == NULL) {
                fc_error(fc, "Compiler bug, cant find ptr class", NULL);
            }
            t->class = idf->item;
            t->bytes = pointer_size;
        }
    }

    if (t->type == type_unknown) {
        //
        IdentifierFor *idf = idf_find_in_scope(scope, id);

        if (idf == NULL) {
            free_type(t);
            return NULL;
        }

        if (idf->type == idfor_enum) {
            t->type = type_enum;
            t->enu = idf->item;
            t->bytes = pointer_size;
        } else if (idf->type == idfor_class) {
            t->type = type_struct;
            t->class = idf->item;
            t->is_pointer = true;
            t->bytes = pointer_size;
            if (t->class->is_number) {
                // t->type = type_number;
                t->is_pointer = false;
                t->allow_math = true;
                t->bytes = t->class->size;
            }
        } else if (idf->type == idfor_type) {
            Type *idt = idf->item;
            *t = *idt;
        }
    }

    if (t->type == type_unknown) {
        free_type(t);
        return NULL;
    }

    return t;
}

Type *fc_create_type_for_enum(Enum *enu) {
    //
    Type *t = init_type();
    t->type = type_enum;
    t->enu = enu;
    t->bytes = pointer_size;
    return t;
}

char *type_to_str(Type *t) {
    //
    if (!t) {
        return strdup("void");
    }
    //
    char *res = malloc(KI_TOKEN_MAX);
    strcpy(res, "");
    if (t->type == type_void_pointer) {
        strcat(res, "ptr");
        return res;
    }
    if (t->npt) {
        strcat(res, "NPT ");
    }
    if (t->nullable) {
        strcat(res, "?");
    }
    if (t->is_pointer) {
        // strcat(res, "*");
    }
    if (t->class) {
        Class *class = t->class;
        strcat(res, class->cname);
    } else if (t->type == type_null) {
        strcat(res, "null");
    } else if (t->type == type_funcref) {
        strcat(res, "funcref (");
        for (int i = 0; i < t->func_arg_types->length; i++) {
            if (i > 0) {
                strcat(res, ", ");
            }
            Type *arg_type = array_get_index(t->func_arg_types, i);
            strcat(res, type_to_str(arg_type));
        }
        strcat(res, ") ");
        strcat(res, type_to_str(t->func_return_type));
    } else {
        strcat(res, "(Unknown class)");
    }
    return res;
}

void fc_type_compatible(FileCompiler *fc, Type *t1, Type *t2) {
    if (!type_compatible(t1, t2)) {
        char *t1s = type_to_str(t1);
        char *t2s = type_to_str(t2);
        printf("Expected: %s\n", t1s);
        printf("Given: %s\n", t2s);
        fc_error(fc, "Types are not compatible", NULL);
    }
}

bool type_compatible(Type *t1, Type *t2) {
    //
    if (t1 == NULL || t2 == NULL) {
        return false;
    }
    if (t1->nullable && t2->type == type_null) {
        return true;
    }
    if (t1->type == type_void_pointer && t2->npt == false) {
        return true;
    }
    if (t2->nullable && !t1->nullable) {
        return false;
    }
    if (t1->is_pointer != t2->is_pointer) {
        return false;
    }
    // if (t1->type == type_number && t2->type == type_enum) {
    //     return true;
    // }
    if (t1->type != t2->type) {
        return false;
    }
    if (t1->type == type_enum) {
        if (t1->enu != t2->enu)
            return false;
    } else if (t1->type == type_struct) {
        if (t1->class->is_number) {
            if (!t2->class->is_number) {
                return false;
            }
        } else {
            if (t1->class != t2->class) {
                return false;
            }
            if (t1->class->generic_hash != t2->class->generic_hash) {
                return false;
            }
        }
    }
    return true;
}

Type *type_generate_generic(Type *type, Type *subtype) {
    //
    Str *uid = str_make("|");
    char *name = type_to_str(subtype);
    str_append_chars(uid, name);
    str_append_chars(uid, "|");

    free(name);

    char *uidchars = str_to_chars(uid);
    free_str(uid);

    char *hash = malloc(33);
    strcpy(hash, "");
    md5(uidchars, hash);
    free(uidchars);

    char *cname = malloc(KI_TOKEN_MAX);
    strcpy(cname, type->class->cname);
    strcat(cname, "__");
    strcat(cname, hash);

    IdentifierFor *idf = map_get(c_identifiers, cname);
    if (!idf) {
        printf("Trying to find:%s\n", cname);
        printf("Known identifiers:\n");
        map_print_keys(c_identifiers);
        die("Compiler bug, could not find Array<String> type");
    }
    Class *gclass = idf->item;
    free(hash);

    Type *t = init_type();
    t->type = type_struct;
    t->class = idf->item;
    t->is_pointer = true;
    t->bytes = pointer_size;
    if (t->class->is_number) {
        t->is_pointer = false;
        t->allow_math = true;
        t->bytes = t->class->size;
    }

    return t;
}