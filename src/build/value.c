
#include "../all.h"

Value *init_value() {
    //
    Value *v = malloc(sizeof(Value));
    v->type = vt_unknown;
    v->return_type = NULL;
    v->item = NULL;
    return v;
}

void free_value(Value *v) {
    //
    free(v);
}

Value *fc_read_value(FileCompiler *fc, Scope *scope, bool readonly, bool sameline, bool allow_space) {
    //
    char *token = malloc(KI_TOKEN_MAX);
    int index = fc->i;
    Value *value = init_value();

    fc_next_token(fc, token, false, false, true);

    if (strcmp(token, "(") == 0) {
        value->type = vt_group;
        Value *subv = fc_read_value(fc, scope, false, false, true);
        value->item = subv;
        value->return_type = subv->return_type;
        fc_expect_token(fc, ")", false, false, true);
    } else if (strcmp(token, "KI_ALLOCATORS") == 0) {
        value->type = vt_var;
        value->item = strdup("KI_ALLOCATORS");
        value->return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "ptr"), NULL);
    } else if (strcmp(token, "KI_ALLOCATORS_MUT") == 0) {
        value->type = vt_var;
        value->item = strdup("KI_ALLOCATORS_MUT");
        value->return_type = fc_identifier_to_type(fc, create_identifier("ki", "async", "Mutex"), NULL);
    } else if (strcmp(token, "null") == 0) {
        value->type = vt_null;
        Type *type = init_type();
        type->type = type_null;
        value->return_type = type;
    } else if (strcmp(token, "true") == 0) {
        value->type = vt_true;
        value->return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "bool"), NULL);
    } else if (strcmp(token, "false") == 0) {
        value->type = vt_false;
        value->return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "bool"), NULL);
    } else if (strcmp(token, "sizeof") == 0) {
        value->type = vt_sizeof;
        value->return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "u32"), NULL);
        fc_expect_token(fc, "(", false, true, false);
        // type or type of var
        IdentifierFor *idf = fc_read_and_get_idf(fc, scope, false, true, true);
        int size = 0;
        if (idf->type == idfor_class) {
            Class *class = idf->item;
            if (class->generic_names != NULL) {
                // Generic class
                class = fc_get_generic_class(fc, class, scope);
            }
            size = class->size;
        } else if (idf->type == idfor_enum) {
            size = 4;
        } else if (idf->type == idfor_func) {
            size = pointer_size;
        } else if (idf->type == idfor_var) {
            Type *type = idf->item;
            size = type->bytes;
        } else if (idf->type == idfor_arg) {
            Type *type = idf->item;
            size = type->bytes;
        } else if (idf->type == idfor_type) {
            Type *type = idf->item;
            size = type->bytes;
        } else {
            fc_error(fc, "cannot determine sizeof this value", NULL);
        }
        value->item = malloc(16);
        sprintf(value->item, "%d", size);
        //
        fc_expect_token(fc, ")", false, true, true);
    } else if (strcmp(token, "cast") == 0) {
        value->type = vt_cast;

        ValueCast *cast = malloc(sizeof(ValueCast));
        cast->value = fc_read_value(fc, scope, false, true, true);

        fc_expect_token(fc, "as", false, true, true);

        cast->as_type = fc_read_type(fc, scope);

        value->item = cast;
        value->return_type = cast->as_type;
    } else if (strcmp(token, "getptrv") == 0) {
        value->type = vt_getptrv;

        ValueCast *cast = malloc(sizeof(ValueCast));
        cast->value = fc_read_value(fc, scope, false, true, true);

        fc_expect_token(fc, "as", false, true, true);

        cast->as_type = fc_read_type(fc, scope);

        value->item = cast;
        value->return_type = cast->as_type;
    } else if (strcmp(token, "getptr") == 0) {
        value->type = vt_getptr;
        value->item = fc_read_value(fc, scope, false, true, true);
        value->return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "ptr"), NULL);
    } else if (strcmp(token, "setptrv") == 0) {
        value->type = vt_setptrv;

        SetPtrValue *cast = malloc(sizeof(ValueCast));
        cast->ptr_value = fc_read_value(fc, scope, false, true, true);

        fc_expect_token(fc, "to", false, true, true);

        cast->to_value = fc_read_value(fc, scope, false, true, true);

        value->item = cast;
        value->return_type = NULL;
    } else if (strcmp(token, "\"") == 0) {
        // String
        Str *str = str_make("");
        char prev_ch = '\0';
        char ch = '\0';
        while (fc->i < fc->content_len) {
            prev_ch = ch;
            ch = fc->content[fc->i];
            fc->i++;
            if (ch == '"' && prev_ch != '\\') {
                break;
            }
            str_append_char(str, ch);
        }

        char *strbody = str_to_chars(str);
        free_str(str);

        char *globname = malloc(64);
        GEN_C++;
        sprintf(globname, "_KI_STRING_%s_%d", fc->hash, GEN_C);

        ValueString *vstr = malloc(sizeof(ValueString));
        vstr->name = globname;
        vstr->body = strbody;

        array_push(fc->strings, vstr);

        value->item = globname;
        value->type = vt_var;
        value->return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "string"), NULL);

    } else if (strcmp(token, "'") == 0) {
        char *str = malloc(3);
        strcpy(str, "");
        char ch = fc_get_char(fc, 0);
        fc->i++;
        str[0] = ch;
        str[1] = '\0';
        if (ch == '\\') {
            ch = fc_get_char(fc, 0);
            fc->i++;
            str[1] = ch;
            str[2] = '\0';
        }

        value->type = vt_char;
        value->item = str;
        value->return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "u32"), NULL);

        fc_expect_token(fc, "'", false, true, false);
        // } else if (strcmp(token, "_") == 0 && fc_get_char(fc, 0) == ':') {
        //   // imported global access
        //   fc->i++;
        //   fc_next_token(fc, token, false, true, false);
        //   if (!is_valid_varname(token)) {
        //     fc_error(fc, "Invalid identifier: '%s'", token);
        //   }

        //   if (strcmp(token, "struct") == 0) {
        //   }

        //   IdentifierFor* idf = map_get(c_identifiers, token);
        //   if (idf == NULL) {
        //     fc_error(fc, "Unknown variable/function: '%s'", token);
        //   }

        //   if (idf->type == idfor_func) {
        //     Function* func = idf->item;
        //     value->type = vt_var;
        //     value->item = strdup(token);
        //     Type* t = init_type();
        //     t->type = type_funcref;
        //     t->func = func;
        //     value->return_type = t;
        //   } else if (idf->type == idfor_var) {
        //     value->type = vt_var;
        //     value->item = strdup(token);
        //     value->return_type = idf->item;
        //   } else {
        //     fc_error(fc, "Unhandled identifier (compiler import bug): '%s'",
        //     token);
        //   }

    } else if (is_valid_number(token) || strcmp(token, "-") == 0) {
        bool negative = strcmp(token, "-") == 0;
        // todo: put negative in value
        if (negative) {
            fc_next_token(fc, token, false, true, false);
            if (!is_valid_number(token)) {
                fc_error(fc, "Invalid number syntax: %s", token);
            }
        }

        // Check hex number
        bool is_hex = false;
        if (strcmp(token, "0") == 0) {
            char ch = fc_get_char(fc, 0);
            if (ch == 'x') {
                fc->i++;
                is_hex = true;
                fc_next_token(fc, token, false, true, false);
                prepend(token, "0x");
                if (!is_valid_hex_number(token)) {
                    fc_error(fc, "Invalid hex number: %s", token);
                }
            }
        }

        if (negative) {
            prepend(token, "-");
        }

        char ch = fc_get_char(fc, 0);
        if (!is_hex && ch == '.') {
            // Float
            fc->i++;
            char *fl = malloc(256);
            strcpy(fl, token);
            strcat(fl, ".");
            int x = strlen(fl);
            char digit = fc_get_char(fc, 0);
            if (is_number(digit)) {
                if (x >= 250) {
                    char msg[100];
                    sprintf(msg,
                            "Float decimals are limited to 250 characters at the moment: "
                            "\"%s\"\n",
                            token);
                    die(msg);
                }
                fl[x] = digit;
                x++;
                fc->i++;
                digit = fc_get_char(fc, 0);
            }
            fl[x] = '\0';
            value->type = vt_number;
            value->item = fl;
            value->return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "f32"), NULL);

        } else {
            // Int
            value->type = vt_number;
            value->item = strdup(token);
            value->return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "i32"), NULL);
        }
    } else if (strcmp(token, "async") == 0) {
        Value *v = fc_read_value(fc, scope, false, true, true);

        if (v->type != vt_func_call) {
            fc_error(fc, "Expected a function call after 'async'", NULL);
        }

        ValueFuncCall *fcall = v->item;
        if (fcall->on->return_type->func_can_error) {
            fc_error(fc, "Cannot do async on function that can throw errors (future feature)", NULL);
        }

        value->type = vt_async;
        value->item = v;
        value->return_type = fc_identifier_to_type(fc, create_identifier("ki", "async", "Task"), NULL);
        value->return_type->func_return_type = v->return_type;

    } else if (strcmp(token, "await") == 0) {
        Value *task = fc_read_value(fc, scope, false, true, true);
        Type *expect_type = fc_identifier_to_type(fc, create_identifier("ki", "async", "Task"), NULL);

        if (task->return_type->class != expect_type->class) {
            fc_error(fc, "Expected a 'channel' value after 'await'", NULL);
        }

        Type *ret_type = task->return_type->func_return_type;

        value->type = vt_await;
        value->item = task;
        value->return_type = ret_type;
    } else if (strcmp(token, "allocator") == 0) {
        Value *sizev = fc_read_value(fc, scope, false, true, true);

        if (sizev->type == vt_sizeof) {
            char *sizec = sizev->item;
            value->item = sizec;
        } else if (sizev->type == vt_number) {
            char *sizec = sizev->item;
            value->item = sizec;
        } else {
            fc_error(fc, "Expected a sizeof value", NULL);
        }

        value->type = vt_allocator;
        value->return_type = fc_identifier_to_type(fc, create_identifier("ki", "mem", "Allocator"), NULL);

    } else if (strcmp(token, "get_threaded") == 0) {
        Identifier *id = fc_read_identifier(fc, false, true, true);
        Scope *idf_scope = fc_get_identifier_scope(fc, fc->scope, id);
        IdentifierFor *idf = idf_find_in_scope(idf_scope, id);

        if (!idf || idf->type != idfor_threaded_var) {
            fc_error(fc, "Cannot find threaded global variable: %s", id->name);
        }

        ThreadedGlobal *tg = idf->item;

        value->type = vt_get_threaded;
        value->item = fc_create_identifier_global_cname(fc, id);
        value->return_type = tg->type;

    } else if (is_valid_varname(token)) {
        IdentifierFor *idf = NULL;
        // if (strcmp(token, "c") == 0 && fc_get_char(fc, 0) == ':') {
        //   // C namespace
        //   fc->i++;  // skip ":"
        //   fc_next_token(fc, token, false, true, false);

        //   if (strcmp(token, "struct") == 0) {
        //     fc_next_token(fc, token, false, true, true);
        //     idf = map_get(c_struct_identifiers, token);
        //   } else {
        //     idf = map_get(c_identifiers, token);
        //   }

        // } else {
        fc->i -= strlen(token);
        Identifier *id = fc_read_identifier(fc, false, true, true);
        Scope *idf_scope = fc_get_identifier_scope(fc, scope, id);
        idf = idf_find_in_scope(idf_scope, id);
        // }
        if (idf == NULL) {
            fc_error(fc, "Unknown variable/function/class/enum: %s", id->name);
        }

        if (idf->type == idfor_func) {
            Function *func = idf->item;
            value->type = vt_var;
            value->item = func->cname;
            Type *t = init_type();
            t->type = type_funcref;
            t->func_arg_types = func->args;
            t->func_return_type = func->return_type;
            t->func_can_error = func->can_error;
            value->return_type = t;
        } else if (idf->type == idfor_enum) {
            fc_expect_token(fc, ".", false, true, false);
            fc_next_token(fc, token, false, true, false);
            if (!is_valid_varname(token)) {
                fc_error(fc, "Invalid enum property: '%s'", token);
            }
            char *prop_name = strdup(token);

            // Enum
            Enum *enu = idf->item;
            char *enuv = map_get(enu->values, prop_name);
            if (!enuv) {
                fc_error(fc, "Unknown enum property: '%s'", prop_name);
            }

            value->type = vt_number;
            value->item = enuv;
            value->return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "i32"), NULL);

        } else if (idf->type == idfor_var) {
            value->type = vt_var;
            value->item = strdup(token);
            value->return_type = idf->item;
        } else if (idf->type == idfor_arg) {
            value->type = vt_arg;
            value->item = strdup(token);
            value->return_type = idf->item;
        } else if (idf->type == idfor_static_var) {
            TokenStaticDeclare *decl = idf->item;
            value->type = vt_var;
            value->item = decl->name;
            value->return_type = decl->scope->return_type;
        } else if (idf->type == idfor_mutex) {
            // todo: check idf_scope is global
            value->type = vt_mutex;
            value->item = fc_create_identifier_global_cname(fc, id);
            value->return_type = fc_identifier_to_type(fc, create_identifier("main", "main", "pthread_mutex_t"), NULL);
        } else if (idf->type == idfor_class) {
            // class init or static func or prop access
            Class *class = idf->item;
            if (class->generic_names != NULL) {
                // Generic class
                Class *gclass = fc_get_generic_class(fc, class, scope);
                class = gclass;
            }
            if (fc_get_char(fc, 0) == '.') {
                // Prop access

                fc->i++;
                fc_next_token(fc, token, false, true, false);
                char *prop_name = strdup(token);

                ValueClassPropAccess *pa = malloc(sizeof(ValueClassPropAccess));
                pa->on = class;
                pa->name = prop_name;
                pa->is_static = true;

                ClassProp *prop = map_get(class->props, prop_name);
                if (!prop) {
                    fc_error(fc, "Unknown property: %s", prop_name);
                }
                if (!prop->is_static) {
                    fc_error(fc, "Trying to access non static property statically: %s", prop_name);
                }

                value->type = vt_prop_access;
                value->item = pa;
                value->return_type = prop->return_type;

            } else {
                // Init func
                fc_expect_token(fc, "{", false, true, true);

                ValueClassInit *ini = malloc(sizeof(ValueClassInit));
                ini->class = class;
                ini->prop_values = map_make();

                // Read prop values
                fc_next_token(fc, token, false, false, true);
                while (strcmp(token, "}") != 0) {
                    char *prop_name = strdup(token);
                    ClassProp *prop = map_get(class->props, prop_name);
                    if (!prop) {
                        fc_error(fc, "Unknown property: %s", prop_name);
                    }
                    if (prop->is_static) {
                        fc_error(fc, "Property is static: %s", prop_name);
                    }
                    if (prop->is_func) {
                        fc_error(fc, "Property is a function: %s", prop_name);
                    }
                    fc_expect_token(fc, ":", false, true, true);

                    Value *value = fc_read_value(fc, scope, false, true, true);

                    fc_next_token(fc, token, false, false, true);
                    if (strcmp(token, ",") == 0) {
                        fc_next_token(fc, token, false, false, true);
                    }

                    map_set(ini->prop_values, prop_name, value);
                }

                //
                value->type = vt_class_init;
                value->item = ini;

                Type *type = init_type();
                type->type = type_struct;
                type->is_pointer = true;
                type->class = class;
                type->bytes = class->size;
                if (type->class->is_number) {
                    type->is_pointer = false;
                    type->allow_math = true;
                }

                value->return_type = type;
            }

            free_id(id);

        } else {
            fc_error(fc, "Unhandled identifier (compiler bug): '%s'", token);
        }
    }

    if (value->type == vt_unknown) {
        fc_error(fc, "Unknown value: '%s'", token);
    }

    fc_next_token(fc, token, true, true, true);
    char ch = fc_get_char(fc, 0);
    while (ch == '.' || ch == '(' || strcmp(token, "+") == 0 || strcmp(token, "-") == 0 || strcmp(token, "*") == 0 || strcmp(token, "/") == 0 || strcmp(token, "%") == 0 || strcmp(token, "<<") == 0 || strcmp(token, ">>") == 0 || strcmp(token, "bitOR") == 0 || strcmp(token, "bitAND") == 0 || strcmp(token, "bitXOR") == 0 || strcmp(token, "++") == 0 || strcmp(token, "--") == 0 || strcmp(token, "<=") == 0 || strcmp(token, ">=") == 0 || strcmp(token, "==") == 0 || strcmp(token, "!=") == 0 || strcmp(token, ">") == 0 || strcmp(token, "<") == 0 || strcmp(token, "&&") == 0 || strcmp(token, "||") == 0) {
        fc_next_token(fc, token, false, true, true);
        //
        if (ch == '.') {
            // Prop access
            if (!value->return_type || (!value->return_type->class && !value->return_type->enu)) {
                fc_error(fc, "Trying to access property on non class/enum value", NULL);
            }
            fc_next_token(fc, token, false, true, false);
            if (!is_valid_varname(token)) {
                fc_error(fc, "Invalid property: '%s'", token);
            }
            char *prop_name = strdup(token);

            // Enum
            if (value->return_type->enu) {
                Enum *enu = value->return_type->enu;
                char *enuv = map_get(enu->values, prop_name);
                if (!enuv) {
                    fc_error(fc, "Unknown enum property: '%s'", prop_name);
                }

                value = init_value();
                value->type = vt_number;
                value->item = enuv;
                value->return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "i32"), NULL);

            } else if (value->return_type->class) {
                // Class
                Class *class = value->return_type->class;
                // printf("pa:%s\n", prop_name);
                // printf("ca:%s\n", class->cname);
                ClassProp *prop = map_get(class->props, prop_name);
                if (!prop) {
                    fc_error(fc, "Unknown property: '%s'", prop_name);
                }
                if (prop->is_static) {
                    fc_error(fc, "Property is static: '%s'", prop_name);
                }

                ValueClassPropAccess *pa = malloc(sizeof(ValueClassPropAccess));
                pa->on = value;
                pa->name = prop_name;
                pa->is_static = false;

                value = init_value();
                value->type = vt_prop_access;
                value->item = pa;
                value->return_type = prop->return_type;
            }

        } else if (ch == '(') {
            // Func call
            value = fc_read_func_call(fc, scope, value);
        } else if (strcmp(token, "+") == 0 || strcmp(token, "-") == 0 || strcmp(token, "*") == 0 || strcmp(token, "/") == 0 || strcmp(token, "<<") == 0 || strcmp(token, ">>") == 0 || strcmp(token, "%") == 0 || strcmp(token, "bitOR") == 0 || strcmp(token, "bitAND") == 0 || strcmp(token, "bitXOR") == 0) {
            ValueOperator *op = malloc(sizeof(ValueOperator));
            op->left = value;
            op->right = fc_read_value(fc, scope, false, false, true);

            if (strcmp(token, "+") == 0) {
                op->type = op_add;
            } else if (strcmp(token, "-") == 0) {
                op->type = op_sub;
            } else if (strcmp(token, "*") == 0) {
                op->type = op_mult;
            } else if (strcmp(token, "/") == 0) {
                op->type = op_div;
            } else if (strcmp(token, "%") == 0) {
                op->type = op_mod;
            } else if (strcmp(token, "bitOR") == 0) {
                op->type = op_bit_OR;
            } else if (strcmp(token, "bitAND") == 0) {
                op->type = op_bit_AND;
            } else if (strcmp(token, "bitXOR") == 0) {
                op->type = op_bit_XOR;
            } else if (strcmp(token, "<<") == 0) {
                op->type = op_bit_shift_left;
            } else if (strcmp(token, ">>") == 0) {
                op->type = op_bit_shift_right;
            }

            Type *return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "i32"), NULL);

            value = init_value();
            value->type = vt_operator;
            value->item = op;
            value->return_type = return_type;
        } else if (strcmp(token, "==") == 0 || strcmp(token, "!=") == 0 || strcmp(token, "<=") == 0 || strcmp(token, ">=") == 0 || strcmp(token, "<") == 0 || strcmp(token, ">") == 0) {
            ValueOperator *op = malloc(sizeof(ValueOperator));
            op->left = value;
            op->right = fc_read_value(fc, scope, false, false, true);

            if (strcmp(token, "==") == 0) {
                op->type = op_eq;
            } else if (strcmp(token, "!=") == 0) {
                op->type = op_neq;
            } else if (strcmp(token, "<=") == 0) {
                op->type = op_lte;
            } else if (strcmp(token, ">=") == 0) {
                op->type = op_gte;
            } else if (strcmp(token, "<") == 0) {
                op->type = op_lt;
            } else if (strcmp(token, ">") == 0) {
                op->type = op_gt;
            }

            Type *return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "bool"), NULL);

            value = init_value();
            value->type = vt_operator;
            value->item = op;
            value->return_type = return_type;
        } else if (strcmp(token, "++") == 0 || strcmp(token, "--") == 0) {
            ValueOperator *op = malloc(sizeof(ValueOperator));
            op->left = value;
            op->right = NULL;

            if (strcmp(token, "++") == 0) {
                op->type = op_incr;
            } else {
                op->type = op_decr;
            }

            value = init_value();
            value->type = vt_operator;
            value->item = op;
            value->return_type = NULL;

        } else if (strcmp(token, "&&") == 0 || strcmp(token, "||") == 0) {
            ValueOperator *op = malloc(sizeof(ValueOperator));
            op->left = value;
            op->right = fc_read_value(fc, scope, false, false, true);

            if (strcmp(token, "&&") == 0) {
                op->type = op_and;
            } else if (strcmp(token, "||") == 0) {
                op->type = op_or;
            }

            Type *return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "bool"), NULL);

            value = init_value();
            value->type = vt_operator;
            value->item = op;
            value->return_type = return_type;
        } else {
            fc_error(fc, "Unhandled operator '%s' (todo)", token);
        }

        //
        ch = fc_get_char(fc, 0);
        fc_next_token(fc, token, true, true, true);
    }

    if (readonly) {
        fc->i = index;
    }

    return value;
}

Value *fc_read_func_call(FileCompiler *fc, Scope *scope, Value *on) {
    if (on->return_type->type != type_funcref) {
        fc_error(fc, "Trying to do a function call on a non function value.", NULL);
    }
    if (on->return_type->nullable) {
        fc_error(fc, "Trying to do a function call on a nullable value.", NULL);
    }

    ValueFuncCall *fcall = malloc(sizeof(ValueFuncCall));
    fcall->on = on;
    fcall->arg_values = array_make(2);
    fcall->error_type = or_none;
    fcall->or_value = NULL;
    fcall->func_scope = NULL;

    Value *value = init_value();
    value->type = vt_func_call;
    value->item = fcall;
    value->return_type = on->return_type->func_return_type;

    Scope *func_scope = scope;
    while (func_scope && func_scope->is_func == false) {
        func_scope = func_scope->parent;
    }
    if (func_scope == NULL) {
        fc_error(fc, "Trying to call function outside a function scope", NULL);
    }

    if (on->return_type->func_can_error)
        func_scope->catch_errors = true;

    if (on->type == vt_prop_access) {
        ValueClassPropAccess *pa = on->item;
        if (!pa->is_static) {
            Value *class_instance_value = pa->on;
            Class *class = class_instance_value->return_type->class;
            ClassProp *prop = map_get(class->props, pa->name);
            if (prop->func) {
                Value *prev_on = on;
                on = init_value();
                on->type = vt_var;
                on->item = prop->func->cname;
                on->return_type = prev_on->return_type;
                fcall->on = on;
                array_push(fcall->arg_values, pa->on);
            }
        }
    }

    char *token = malloc(KI_TOKEN_MAX);
    fc_next_token(fc, token, true, false, true);
    while (strcmp(token, ")") != 0) {
        Value *argv = fc_read_value(fc, scope, false, false, true);
        array_push(fcall->arg_values, argv);
        fc_next_token(fc, token, true, false, true);
        if (strcmp(token, ",") == 0) {
            fc_next_token(fc, token, false, false, true);
            fc_next_token(fc, token, true, false, true);
        }
    }
    fc_next_token(fc, token, false, false, true);

    if (fcall->arg_values->length < fcall->on->return_type->func_arg_types->length) {
        fc_error(fc, "Too few arguments", NULL);
    }

    // Check error handling
    if (on->return_type->func_can_error) {
        //
        fcall->func_scope = func_scope;
        //
        fc_expect_token(fc, "or", false, true, true);
        fc_next_token(fc, token, false, true, true);
        if (strcmp(token, "pass") == 0) {
            if (func_scope->func->can_error == false) {
                fc_error(fc,
                         "Trying to pass an error in a function that has no error "
                         "return type",
                         NULL);
            }
            fcall->error_type = or_pass;
        } else if (strcmp(token, "value") == 0) {
            fcall->error_type = or_value;
            fcall->or_value = fc_read_value(fc, scope, false, true, true);
            if (fcall->or_value->return_type->nullable) {
                value->return_type->nullable = true;
            }
            fc_type_compatible(fc, value->return_type, fcall->or_value->return_type);
        } else if (strcmp(token, "return") == 0) {
            fcall->error_type = or_return;
            fcall->or_value = fc_read_value(fc, scope, false, true, true);
            if (fcall->or_value->return_type->nullable) {
                value->return_type->nullable = true;
            }
            fc_type_compatible(fc, func_scope->return_type, fcall->or_value->return_type);
        } else if (strcmp(token, "throw") == 0) {
            fcall->error_type = or_throw;
            fc_next_token(fc, token, false, true, true);
            fcall->throw_msg = strdup(token);
        } else {
            fc_error(fc,
                     "Invalid error handling method: '%s' , valid options: pass, "
                     "throw, value, return",
                     token);
        }
    }

    //
    return value;
}