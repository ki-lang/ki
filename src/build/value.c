
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

    if (g_verbose_all) {
        printf("Read value at %d in %s\n", index, fc->ki_filepath);
    }

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
        type->nullable = true;
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
        if (!idf) {
            fc_error(fc, "Unknown identifier", NULL);
        }
        int size = 0;
        if (idf->type == idfor_class) {
            Class *class = idf->item;
            if (class->generic_names != NULL && class->generic_hash == NULL) {
                // Generic class
                class = fc_get_generic_class(fc, class, scope);
            }
            size = class->size;
        } else if (idf->type == idfor_enum) {
            size = 4;
        } else if (idf->type == idfor_func) {
            size = pointer_size;
        } else if (idf->type == idfor_local_var) {
            LocalVar *lv = idf->item;
            size = lv->type->bytes;
        } else if (idf->type == idfor_arg) {
            LocalVar *lv = idf->item;
            size = lv->type->bytes;
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
    } else if (strcmp(token, "?") == 0) {
        // ?Value<...>
        Value *v = fc_read_value(fc, scope, false, true, true);

        // Generate ?Value<T> type
        Type *opt_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "Value"), NULL);
        Array *subtypes = array_make(2);
        array_push(subtypes, v->return_type);
        Class *gclass = fc_get_generic_class_by_hash(opt_type->class, subtypes);
        // array_free(subtypes); // shouldnt free values

        fc_depends_on(fc, gclass->fc);

        Type *t = init_type();
        t->type = type_struct;
        t->class = gclass;
        t->is_pointer = true;
        t->bytes = pointer_size;
        t->allow_math = false;
        t->nullable = true;

        value->type = vt_nullable_value;
        value->item = v;
        value->return_type = t;

    } else if (strcmp(token, "cast") == 0) {

        ValueCast *cast = malloc(sizeof(ValueCast));
        cast->value = fc_read_value(fc, scope, false, true, true);

        fc_expect_token(fc, "as", false, true, true);

        cast->as_type = fc_read_type(fc, scope);

        value->type = vt_cast;
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

        Type *ptr_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "ptr"), NULL);
        fc_type_compatible(fc, ptr_type, cast->ptr_value->return_type);

        fc_expect_token(fc, "to", false, true, true);

        cast->to_value = fc_read_value(fc, scope, false, true, true);

        if (!cast->to_value->return_type) {
            fc_error(fc, "Value has no return type", NULL);
        }

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
            if (ch == '#' && fc->content[fc->i] == '{') {
                // macro token
                fc->i++;
                fc_next_token(fc, token, false, true, true);
                if (is_valid_varname(token)) {
                    Identifier *id = create_identifier(NULL, NULL, token);
                    IdentifierFor *idf = idf_find_in_scope(scope, id);
                    if (!idf || idf->type != idfor_macro_token) {
                        fc_error(fc, "Invalid macro token: %s", token);
                    }
                    char *value = idf->item;
                    str_append_chars(str, value);
                    fc_expect_token(fc, "}", false, true, true);
                }
                continue;
            }
            if (ch == '"' && prev_ch != '\\') {
                break;
            }
            str_append_char(str, ch);
        }

        char *strbody = str_to_chars(str);
        free_str(str);

        char *globname = malloc(64);
        fc->var_bufc++;
        sprintf(globname, "_KI_STRING_%s_%d", fc->hash, fc->var_bufc);

        ValueString *vstr = malloc(sizeof(ValueString));
        vstr->name = globname;
        vstr->body = strbody;

        array_push(fc->strings, vstr);

        value->item = globname;
        value->type = vt_var;
        value->return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "String"), NULL);

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
            while (is_number(digit)) {
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

        ch = fc_get_char(fc, 0);
        if (ch == '#') {
            fc->i++;
            // Cast
            Type *type = fc_read_type(fc, scope);
            if (!type->class || !type->class->is_number) {
                fc_error(fc, "Not a number type", NULL);
            }
            value->return_type = type;
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

    } else if (strcmp(token, "PTRSIZE") == 0) {
        char *num = malloc(16);
        sprintf(num, "%d", pointer_size);

        value->type = vt_number;
        value->item = num;
        value->return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "u16"), NULL);

    } else if (is_valid_varname(token)) {

        IdentifierFor *idf = NULL;
        fc->i -= strlen(token);
        Identifier *id = fc_read_identifier(fc, false, true, true);
        Scope *idf_scope = fc_get_identifier_scope(fc, scope, id);
        idf = idf_find_in_scope(idf_scope, id);
        if (idf == NULL) {
            if (id->namespace) {
                printf("Found '%d' identifiers within this namespace '%s' : ", idf_scope->identifiers->keys->length, id->namespace);
                for (int i = 0; i < idf_scope->identifiers->keys->length; i++) {
                    char *key = array_get_index(idf_scope->identifiers->keys, i);
                    if (i > 0) {
                        printf(", ");
                    }
                    printf("%s", key);
                }
                printf("\n");
            }
            fc_error(fc, "Unknown variable/function/class/enum: %s", id->name);
        }

        if (idf->type == idfor_func) {
            Function *func = idf->item;

            value->type = vt_var;
            value->item = func->cname;

            Type *t = init_type();
            t->type = type_funcref;
            t->is_pointer = true;
            t->func_arg_types = func->arg_types;
            t->func_return_type = func->return_type;
            t->func_can_error = func->can_error;
            value->return_type = t;

            fc_depends_on(fc, func->fc);

            if (fc->current_func_scope) {
                array_push_unique(func->called_by, fc->current_func_scope->func);
            } else if (func->accesses_globals) {
                fc_error(fc, "Globally accessing function that accesses other globals (not allowed because of race conditions)", NULL);
            }

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

            fc_depends_on(fc, enu->fc);

        } else if (idf->type == idfor_local_var) {
            LocalVar *lv = idf->item;
            value->type = vt_var;
            value->item = lv->gen_name;
            value->return_type = lv->type;
        } else if (idf->type == idfor_threaded_global) {
            GlobalVar *gv = idf->item;
            value->type = vt_threaded_global;
            value->item = gv;
            value->return_type = gv->return_type;

            fc_depends_on(fc, gv->fc);

            if (fc->current_func_scope) {
                fc->current_func_scope->func->accesses_globals = true;
            } else {
                fc_error(fc, "Accessing globals outside a function scope is not allowed", NULL);
            }

        } else if (idf->type == idfor_shared_global) {
            GlobalVar *gv = idf->item;
            value->type = vt_shared_global;
            value->item = gv;
            value->return_type = gv->return_type;

            fc_depends_on(fc, gv->fc);

            if (fc->current_func_scope) {
                fc->current_func_scope->func->accesses_globals = true;
            } else {
                fc_error(fc, "Accessing globals outside a function scope is not allowed", NULL);
            }

        } else if (idf->type == idfor_arg) {
            LocalVar *lv = idf->item;
            value->type = vt_arg;
            value->item = lv->name;
            value->return_type = lv->type;
        } else if (idf->type == idfor_class) {
            // class init or static func or prop access
            Class *class = idf->item;
            if (class->generic_names != NULL && class->generic_hash == NULL) {
                // Generic class
                Class *gclass = fc_get_generic_class(fc, class, scope);
                class = gclass;
            }

            fc_depends_on(fc, class->fc);

            for (int i = 0; i < class->traits->length; i++) {
                Trait *tr = array_get_index(class->traits, i);
                fc_depends_on(fc, tr->fc);
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

                if (prop->is_func) {
                    if (fc->current_func_scope) {
                        array_push_unique(prop->func->called_by, fc->current_func_scope->func);
                    } else if (prop->func->accesses_globals) {
                        fc_error(fc, "Globally accessing function that accesses other globals (not allowed because of race conditions)", NULL);
                    }
                }

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

                    fc_type_compatible(fc, prop->return_type, value->return_type);

                    fc_next_token(fc, token, false, false, true);
                    if (strcmp(token, ",") == 0) {
                        fc_next_token(fc, token, false, false, true);
                    }

                    map_set(ini->prop_values, prop_name, value);
                }
                // Check if all props are filled in
                for (int i = 0; i < class->props->keys->length; i++) {
                    char *name = array_get_index(class->props->keys, i);
                    ClassProp *prop = array_get_index(class->props->values, i);
                    if (!prop->is_func && !prop->is_static && prop->default_value == NULL) {
                        Value *v = map_get(ini->prop_values, name);
                        if (v == NULL) {
                            fc_error(fc, "Missing property '%s'", name);
                        }
                    }
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
    while (ch == '.' || ch == '(' || strcmp(token, "+") == 0 || strcmp(token, "-") == 0 || strcmp(token, "*") == 0 || strcmp(token, "/") == 0 || strcmp(token, "%") == 0 || strcmp(token, "<<") == 0 || strcmp(token, "bitOR") == 0 || strcmp(token, "bitAND") == 0 || strcmp(token, "bitXOR") == 0 || strcmp(token, "++") == 0 || strcmp(token, "--") == 0 || strcmp(token, "<=") == 0 || strcmp(token, ">=") == 0 || strcmp(token, "==") == 0 || strcmp(token, "!=") == 0 || strcmp(token, ">") == 0 || strcmp(token, "<") == 0 || strcmp(token, "&&") == 0 || strcmp(token, "||") == 0 || strcmp(token, "or") == 0) {
        fc_next_token(fc, token, false, true, true);
        if (strcmp(token, ">") == 0 && fc_get_char(fc, 0) == '>') {
            strcpy(token, ">>");
            fc->i++;
        }
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

                if (fc_get_char(fc, 0) == '#') {
                    fc->i++;
                    value->return_type = fc_read_type(fc, scope);
                    if (value->return_type->type == type_number) {
                        fc_error(fc, "Must be a number type", NULL);
                    }
                } else {
                    Type *type = init_type();
                    type->type = type_enum;
                    type->enu = enu;
                    value->return_type = type;
                }

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
                if (value->return_type->nullable) {
                    fc_error(fc, "Accessing property on value with nullable type is not allowed", NULL);
                }

                Scope *class_scope = get_class_scope(scope);
                if (!class_scope || class_scope->class != class) {
                    if (prop->access_type == acct_private) {
                        fc_error(fc, "Cannot access private property outside its class", NULL);
                    }
                }

                ValueClassPropAccess *pa = malloc(sizeof(ValueClassPropAccess));
                pa->on = value;
                pa->name = prop_name;
                pa->is_static = false;

                value = init_value();
                value->type = vt_prop_access;
                value->item = pa;
                value->return_type = prop->return_type;

                if (prop->is_func) {
                    if (fc->current_func_scope) {
                        array_push_unique(prop->func->called_by, fc->current_func_scope->func);
                    } else if (prop->func->accesses_globals) {
                        fc_error(fc, "Globally accessing function that accesses other globals (not allowed because of race conditions)", NULL);
                    }
                }
            }

        } else if (ch == '(') {
            // Func call
            value = fc_read_func_call(fc, scope, value);
        } else if (strcmp(token, "+") == 0 || strcmp(token, "-") == 0 || strcmp(token, "*") == 0 || strcmp(token, "/") == 0 || strcmp(token, "<<") == 0 || strcmp(token, ">>") == 0 || strcmp(token, "%") == 0 || strcmp(token, "bitOR") == 0 || strcmp(token, "bitAND") == 0 || strcmp(token, "bitXOR") == 0) {

            Value *right = fc_read_value(fc, scope, false, false, true);
            int optype;

            char *fn = NULL;
            if (strcmp(token, "+") == 0) {
                optype = op_add;
                fn = "__add";
            } else if (strcmp(token, "-") == 0) {
                optype = op_sub;
            } else if (strcmp(token, "*") == 0) {
                optype = op_mult;
            } else if (strcmp(token, "/") == 0) {
                optype = op_div;
            } else if (strcmp(token, "%") == 0) {
                optype = op_mod;
            } else if (strcmp(token, "bitOR") == 0) {
                optype = op_bit_OR;
            } else if (strcmp(token, "bitAND") == 0) {
                optype = op_bit_AND;
            } else if (strcmp(token, "bitXOR") == 0) {
                optype = op_bit_XOR;
            } else if (strcmp(token, "<<") == 0) {
                optype = op_bit_shift_left;
            } else if (strcmp(token, ">>") == 0) {
                optype = op_bit_shift_right;
            }

            // If "+" and left or right is string, convert the other one to string if not string yet
            Type *string_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "String"), NULL);
            if (optype == op_add) {
                Class *left_class = value->return_type->class;
                Class *right_class = right->return_type->class;
                if (left_class != NULL && right_class != NULL && left_class != right_class && (left_class == string_type->class || right_class == string_type->class)) {
                    bool left_is_string = left_class == string_type->class;
                    Value *non_str_value = !left_is_string ? value : right;
                    ClassProp *prop = map_get(non_str_value->return_type->class->props, "__string");

                    if (prop) {
                        ValueFuncCall *fcall = value_generate_func_call(prop->func);

                        array_push(fcall->arg_values, non_str_value);

                        Value *nv = init_value();
                        nv->type = vt_func_call;
                        nv->item = fcall;
                        nv->return_type = prop->func->return_type;

                        if (left_is_string) {
                            right = nv;
                        } else {
                            value = nv;
                        }
                    }
                }
            }

            ClassProp *prop = NULL;
            if (fn && value->return_type->class) {
                prop = map_get(value->return_type->class->props, fn);
            }
            if (prop) {

                if (right->return_type->class != value->return_type->class) {
                    fc_error(fc, "left & right value must be the same type", fn);
                }
                if (!prop->is_func) {
                    fc_error(fc, "%s is not a function", fn);
                }

                ValueFuncCall *fcall = value_generate_func_call(prop->func);

                array_push(fcall->arg_values, value);
                array_push(fcall->arg_values, right);

                value = init_value();
                value->type = vt_func_call;
                value->item = fcall;
                value->return_type = prop->func->return_type;
            } else {

                if (!value->return_type->allow_math) {
                    fc_error(fc, "Cannot use this operator on the left-side value", fn);
                }
                if (!right->return_type->allow_math) {
                    fc_error(fc, "Cannot use this operator with the right-side value", fn);
                }

                ValueOperator *op = malloc(sizeof(ValueOperator));
                op->type = optype;
                op->left = value;
                op->right = right;

                Type *return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "i32"), NULL);

                value = init_value();
                value->type = vt_operator;
                value->item = op;
                value->return_type = return_type;
            }
        } else if (strcmp(token, "==") == 0 || strcmp(token, "!=") == 0 || strcmp(token, "<=") == 0 || strcmp(token, ">=") == 0 || strcmp(token, "<") == 0 || strcmp(token, ">") == 0) {
            Value *right = fc_read_value(fc, scope, false, false, true);

            int optype = -1;
            char *fn = NULL;
            if (strcmp(token, "==") == 0) {
                optype = op_eq;
                fn = "__eq";
            } else if (strcmp(token, "!=") == 0) {
                optype = op_neq;
                fn = "__neq";
            } else if (strcmp(token, "<=") == 0) {
                optype = op_lte;
                fn = "__lte";
            } else if (strcmp(token, ">=") == 0) {
                optype = op_gte;
                fn = "__gte";
            } else if (strcmp(token, "<") == 0) {
                optype = op_lt;
                fn = "__lt";
            } else if (strcmp(token, ">") == 0) {
                optype = op_gt;
                fn = "__gt";
            }

            ClassProp *eq_prop = NULL;
            if (value->return_type->class && right->return_type->class == value->return_type->class) {
                eq_prop = map_get(value->return_type->class->props, fn);
            }

            if (eq_prop) {
                if (!eq_prop->is_func) {
                    fc_error(fc, "%s is not a function", fn);
                }

                ValueFuncCall *fcall = value_generate_func_call(eq_prop->func);

                array_push(fcall->arg_values, value);
                array_push(fcall->arg_values, right);

                value = init_value();
                value->type = vt_func_call;
                value->item = fcall;
                value->return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "bool"), NULL);
            } else {
                ValueOperator *op = malloc(sizeof(ValueOperator));
                op->left = value;
                op->right = right;
                op->type = optype;

                value = init_value();
                value->type = vt_operator;
                value->item = op;
                value->return_type = fc_identifier_to_type(fc, create_identifier("ki", "type", "bool"), NULL);
            }

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
        } else if (strcmp(token, "or") == 0) {

            if (!value->return_type->nullable) {
                fc_error(fc, "Using 'or' on non-nullable value", NULL);
            }

            ValueOperator *op = malloc(sizeof(ValueOperator));
            op->type = op_null_or;
            op->left = value;
            op->right = fc_read_value(fc, scope, false, false, true);

            if (op->right->return_type->nullable) {
                fc_error(fc, "Right-side value cannot be nullable", NULL);
            }

            fc_type_compatible(fc, op->left->return_type, op->right->return_type);

            value = init_value();
            value->type = vt_null_or;
            value->item = op;
            value->return_type = op->right->return_type;
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
    fcall->ort = NULL;

    Value *value = init_value();
    value->type = vt_func_call;
    value->item = fcall;
    value->return_type = on->return_type->func_return_type;

    Scope *func_scope = scope;
    while (func_scope && func_scope->is_func == false) {
        func_scope = func_scope->parent;
    }
    // if (func_scope == NULL) {
    //     fc_error(fc, "Trying to call function outside a function scope", NULL);
    // }

    if (func_scope && on->return_type->func_can_error)
        func_scope->catch_errors = true;

    Array *func_args = fcall->on->return_type->func_arg_types;

    if (on->type == vt_prop_access) {
        ValueClassPropAccess *pa = on->item;
        Class *class = NULL;
        if (pa->is_static) {
            class = pa->on;
        } else {
            Value *class_instance_value = pa->on;
            class = class_instance_value->return_type->class;
        }
        ClassProp *prop = map_get(class->props, pa->name);

        if (prop->is_func) {

            func_args = prop->func->arg_types;

            if (!prop->is_static) {
                Value *prev_on = on;
                on = init_value();
                on->type = vt_var;
                on->item = prop->func->cname;
                on->return_type = prev_on->return_type;
                fcall->on = on;
                array_push(fcall->arg_values, pa->on);
            }
        } else {
            // funcref property
            // func_args = prop->return_type->func_arg_types;
        }
    }
    // printf("on: %s\n", type_to_str(fcall->on->return_type));

    if (!func_args) {
        fc_error(fc, "Function call on non-function", NULL);
    }

    char *token = malloc(KI_TOKEN_MAX);
    fc_next_token(fc, token, true, false, true);
    int i = fcall->arg_values->length; // start from 0 or 1, because "this" might be the first arg
    while (strcmp(token, ")") != 0) {
        Value *argv = fc_read_value(fc, scope, false, false, true);
        // Type check
        Type *arg_type = array_get_index(func_args, i);
        if (!arg_type) {
            fc_error(fc, "Too many arguments", NULL);
        }
        fc_type_compatible(fc, arg_type, argv->return_type);
        //
        array_push(fcall->arg_values, argv);
        fc_next_token(fc, token, true, false, true);
        if (strcmp(token, ",") == 0) {
            fc_next_token(fc, token, false, false, true);
            fc_next_token(fc, token, true, false, true);
        }
        i++;
    }
    fc_next_token(fc, token, false, false, true);

    if (fcall->arg_values->length < fcall->on->return_type->func_arg_types->length) {
        fc_error(fc, "Too few arguments", NULL);
    }
    if (fcall->arg_values->length > fcall->on->return_type->func_arg_types->length) {
        fc_error(fc, "Too many arguments...", NULL);
    }

    // Check error handling
    if (on->return_type->func_can_error) {
        //
        fc_expect_token(fc, "or", false, true, true);

        OrToken *ort = fc_read_or_token(fc, scope, value->return_type, token, true);
        fcall->ort = ort;

        if (ort->vscope) {
            // type check scope
            if (ort->type == or_value) {
                // or value
                if (ort->vscope->vscope_return_type->nullable) {
                    if (!value->return_type->is_pointer) {
                        fc_error(fc, "The or-value is nullable, but the function return type is not a pointer/class type", NULL);
                    }
                    value->return_type = fc_type_make_nullable_copy(fc, value->return_type);
                }
                fc_type_compatible(fc, value->return_type, ort->vscope->vscope_return_type);
            }
        } else if (ort->value) {
            if (ort->type == or_value) {
                // or value
                if (ort->value->return_type->nullable) {
                    if (!value->return_type->is_pointer) {
                        fc_error(fc, "The or-value is nullable, but the function return type is not a pointer/class type", NULL);
                    }
                    value->return_type = fc_type_make_nullable_copy(fc, value->return_type);
                }
            } else if (ort->type == or_return) {
                // or return
                Scope *func_scope = get_func_scope(scope);
                if (func_scope->func->return_type) {
                    fc_type_compatible(fc, func_scope->func->return_type, ort->value->return_type);
                }
            }
        }
    }

    //
    return value;
}

ValueFuncCall *value_generate_func_call(Function *func) {

    Value *on = init_value();
    on->type = vt_var;
    on->item = func->cname;
    Type *t = init_type();
    t->type = type_funcref;
    t->is_pointer = true;
    t->func_arg_types = func->arg_types;
    t->func_return_type = func->return_type;
    t->func_can_error = func->can_error;
    on->return_type = t;

    ValueFuncCall *fcall = malloc(sizeof(ValueFuncCall));
    fcall->on = on;
    fcall->arg_values = array_make(2);
    fcall->ort = NULL;

    return fcall;
}
