
#include "../all.h"

void fc_parse_macro(FileCompiler *fc, Scope *scope, char *token) {
    fc_next_token(fc, token, false, true, false);
    bool end = false;
    if (strcmp(token, "/") == 0) {
        end = true;
        fc_next_token(fc, token, false, true, false);
        // memmove(token + 1, token, strlen(token) + 1);
        // token[0] = '\\';
    }
    if (token[0] == '\0') {
        fc_error(fc, "Dont put a space after a '#'", NULL);
    }

    if (strcmp(token, "header") == 0) {
        // ki header
        fc_read_header_token(fc);
    } else if (strcmp(token, "link") == 0) {
        // Add to linker
        fc_read_link_token(fc);
    } else if (!end && strcmp(token, "tag") == 0) {
        // Set tag
        if (fc->macro_tag) {
            fc_error(fc, "There is already an active tag", NULL);
        }
        fc_next_token(fc, token, false, true, true);
        if (!is_valid_varname(token)) {
            fc_error(fc, "Invalid tag name syntax", token);
        }
        fc->macro_tag = strdup(token);
    } else if (end && strcmp(token, "tag") == 0) {
        // Clear tag
        fc->macro_tag = NULL;
    } else if (!end && strcmp(token, "props") == 0) {
        // Loop class properties
        Identifier *id = fc_read_identifier(fc, false, true, true);
        Scope *idf_scope = fc_get_identifier_scope(fc, scope, id);
        IdentifierFor *idf = idf_find_in_scope(idf_scope, id);
        if (!idf || idf->type != idfor_class) {
            fc_error(fc, "Identfier must be a class", NULL);
        }
        Class *class = idf->item;
        PropLoop *loop = malloc(sizeof(PropLoop));
        loop->filter = NULL;
        loop->class = class;
        array_push(fc->macro_prop_loops, loop);

        fc_expect_token(fc, "as", false, true, true);

        fc_next_token(fc, token, false, true, true);
        if (!is_valid_varname(token) || map_get(scope->identifiers, token)) {
            fc_error(fc, "Invalid or already used identifier for property name: %s", token);
        }
        loop->name_id = strdup(token);

        fc_next_token(fc, token, false, true, true);
        if (!is_valid_varname(token) || map_get(scope->identifiers, token)) {
            fc_error(fc, "Invalid or already used identifier for property type: %s", token);
        }
        loop->type_id = strdup(token);

        fc_next_token(fc, token, false, true, true);
        if (is_valid_varname(token)) {
            if (map_get(scope->identifiers, token)) {
                fc_error(fc, "Invalid or already used identifier for #props tag-filter: %s", token);
            }
            loop->filter = strdup(token);
        }

        loop->fc_i = fc->i;
        loop->prop_index = 0;

        ClassProp *prop = NULL;
        char *prop_name = NULL;
        for (int i = 0; i < class->props->keys->length; i++) {
            char *n = array_get_index(class->props->keys, i);
            ClassProp *p = array_get_index(class->props->values, i);
            if (!loop->filter || (p->macro_tag && strcmp(loop->filter, p->macro_tag) == 0)) {
                prop = p;
                prop_name = n;
                loop->prop_index = i;
                break;
            }
        }

        if (prop) {
            IdentifierFor *idf = init_idf();
            idf->type = idfor_macro_token;
            idf->item = prop_name;
            map_set(scope->identifiers, loop->name_id, idf);
            //
            idf = init_idf();
            idf->type = idfor_type;
            idf->item = prop->return_type;
            map_set(scope->identifiers, loop->type_id, idf);
        } else {
            fc_skip_macro(fc);
        }
        //
    } else if (end && strcmp(token, "props") == 0) {
        // End prop loop
        if (fc->macro_prop_loops->length == 0) {
            fc_error(fc, "Not inside a #props loop", NULL);
        }
        PropLoop *loop = array_get_index(fc->macro_prop_loops, fc->macro_prop_loops->length - 1);
        Class *class = loop->class;

        ClassProp *prop = NULL;
        char *prop_name = NULL;
        for (int i = loop->prop_index + 1; i < class->props->keys->length; i++) {
            char *n = array_get_index(class->props->keys, i);
            ClassProp *p = array_get_index(class->props->values, i);
            if (!loop->filter || (p->macro_tag && strcmp(loop->filter, p->macro_tag) == 0)) {
                prop = p;
                prop_name = n;
                loop->prop_index = i;
                break;
            }
        }

        if (prop) {
            IdentifierFor *idf = map_get(scope->identifiers, loop->name_id);
            idf->item = prop_name;
            idf = map_get(scope->identifiers, loop->type_id);
            idf->item = prop->return_type;

            fc->i = loop->fc_i;
        } else {
            // free everything
            IdentifierFor *idf = map_get(scope->identifiers, loop->name_id);
            free(loop->name_id);
            free_idf(idf);
            idf = map_get(scope->identifiers, loop->type_id);
            free(loop->type_id);
            free_idf(idf);
            //
            free(loop);
            array_pop(fc->macro_prop_loops);
        }
        //
    } else if (strcmp(token, "if") == 0) {
        bool result = fc_resolve_macro_if_value(fc, scope);
        array_push(fc->macro_results, result ? "1" : "0");
        if (!result) {
            fc_skip_macro(fc);
        }
    } else if (strcmp(token, "elif") == 0) {
        if (fc->macro_results->length == 0) {
            fc_error(fc, "Unexcepted else if, no previous if statement", NULL);
        }
        char *last_result = array_pop(fc->macro_results);
        bool result = fc_resolve_macro_if_value(fc, scope);
        array_push(fc->macro_results, (last_result[0] == '1' || result) ? "1" : "0");
        if (last_result[0] == '1' || !result) {
            fc_skip_macro(fc);
        }
    } else if (strcmp(token, "else") == 0) {
        if (fc->macro_results->length == 0) {
            fc_error(fc, "Unexcepted else, no previous if statement", NULL);
        }
        char *last_result = array_pop(fc->macro_results);
        bool result = last_result[0] != '1';
        array_push(fc->macro_results, (last_result[0] == '1' || result) ? "1" : "0");
        if (!result) {
            fc_skip_macro(fc);
        }
    } else if (strcmp(token, "end") == 0) {
        if (fc->macro_results->length == 0) {
            fc_error(fc, "Unexcepted end if, no previous if statement", NULL);
        }
        char *last_result = array_pop(fc->macro_results);
    } else {
        fc_error(fc, "Unknown token: #%s", token);
    }
}

bool fc_resolve_macro_if_value(FileCompiler *fc, Scope *scope) {
    //
    char *token = malloc(KI_TOKEN_MAX);

    fc_next_token(fc, token, false, true, true);

    if (!is_valid_varname(token)) {
        fc_error(fc, "Invalid macro variable name: '%s'", token);
    }

    bool result = false;

    if (strcmp(token, "IS_SIGNED_NUMBER_CLASS") == 0 || strcmp(token, "IS_UNSIGNED_NUMBER_CLASS") == 0) {
        if (!allow_new_namespaces) {
            Type *type = fc_read_type(fc, scope);
            Class *class = type->class;
            if (class && class->is_number && class->is_unsigned == (strcmp(token, "IS_UNSIGNED_NUMBER_CLASS") == 0 ? true : false)) {
                return true;
            }
        } else {
            fc_skip_until_char(fc, '\n');
        }
    } else if (strcmp(token, "IS_NULLABLE_TYPE") == 0) {
        if (!allow_new_namespaces) {
            Type *type = fc_read_type(fc, scope);
            if (type->nullable)
                return true;
        } else {
            fc_skip_until_char(fc, '\n');
        }
    } else if (strcmp(token, "IS_REFCOUNTED_TYPE") == 0) {
        if (!allow_new_namespaces) {
            Type *type = fc_read_type(fc, scope);
            Class *class = type->class;
            if (class && class->ref_count)
                return true;
        } else {
            fc_skip_until_char(fc, '\n');
        }
    } else {
        // Check if var exists and equals to "1"
        char *value = map_get(macro_defines, token);
        if (!value) {
            fc_error(fc, "Unknown macro value: '%s'", token);
        }
        result = strcmp(value, "1") == 0;

        fc_next_token(fc, token, true, true, true);
        if (strcmp(token, "=") == 0) {
            fc_next_token(fc, token, false, true, true);
            fc_next_token(fc, token, false, true, true);
            if (result) {
                result = strcmp(value, token) == 0;
            }
        }
    }

    fc_next_token(fc, token, true, true, true);
    if (strcmp(token, "&&") == 0) {
        fc_next_token(fc, token, false, true, true);
        result = result && fc_resolve_macro_if_value(fc, scope);
    } else if (strcmp(token, "||") == 0) {
        fc_next_token(fc, token, false, true, true);
        bool next = fc_resolve_macro_if_value(fc, scope);
        result = result || next;
    }

    free(token);

    return result;
}