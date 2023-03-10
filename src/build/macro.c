
#include "../all.h"

MacroScope *init_macro_scope(Allocator *alc) {
    //
    MacroScope *ms = al(alc, sizeof(MacroScope));
    ms->parent = NULL;
    ms->identifiers = map_make(alc);
    return ms;
}

void read_macro(Fc *fc, Allocator *alc, Scope *scope) {
    //
    char *token = fc->token;
    tok(fc, token, true, false);

    if (strcmp(token, "if") == 0) {
        MacroScope *mc = fc->current_macro_scope;
        bool result = macro_resolve_if_value(fc, scope, mc);

        mc = init_macro_scope(alc);
        mc->parent = fc->current_macro_scope;
        fc->current_macro_scope = mc;

        map_set(mc->identifiers, "MACRO_IF_RESULT", result ? "1" : "0");

        if (!result) {
            skip_macro_if(fc);
        }
    } else if (strcmp(token, "elif") == 0) {
        MacroScope *mc = fc->current_macro_scope;
        char *last = map_get(mc->identifiers, "MACRO_IF_RESULT");
        if (!last) {
            sprintf(fc->sbuf, "Using #elif without a #if before it");
            fc_error(fc);
        }
        if (strcmp(last, "1") == 0) {
            skip_macro_if(fc);
        } else {
            MacroScope *mc = fc->current_macro_scope;
            bool result = macro_resolve_if_value(fc, scope, mc);
            map_set(mc->identifiers, "MACRO_IF_RESULT", result ? "1" : "0");
            if (!result) {
                skip_macro_if(fc);
            }
        }
    } else if (strcmp(token, "else") == 0) {
        MacroScope *mc = fc->current_macro_scope;
        char *last = map_get(mc->identifiers, "MACRO_IF_RESULT");
        if (!last) {
            sprintf(fc->sbuf, "Using #else without a #if before it");
            fc_error(fc);
        }
        if (strcmp(last, "1") == 0) {
            skip_macro_if(fc);
        }
    } else if (strcmp(token, "end") == 0) {
        MacroScope *mc = fc->current_macro_scope;
        char *last = map_get(mc->identifiers, "MACRO_IF_RESULT");
        if (!last) {
            sprintf(fc->sbuf, "Using #end without a #if before it");
            fc_error(fc);
        }
        fc->current_macro_scope = mc->parent;
        mc = fc->current_macro_scope;
    } else {
        sprintf(fc->sbuf, "Unknown macro token: '%s'", token);
        fc_error(fc);
    }
}

bool macro_resolve_if_value(Fc *fc, Scope *scope, MacroScope *mc) {
    //
    char *token = fc->token;
    tok(fc, token, true, true);

    bool result = false;

    if (strcmp(token, "(") == 0) {
        result = macro_resolve_if_value(fc, scope, mc);
        tok_expect(fc, ")", true, true);
    } else if (strcmp(token, "!") == 0) {
        result = macro_resolve_if_value(fc, scope, mc);
        result = !result;
    } else if (strcmp(token, "IS_REFCOUNTED_TYPE") == 0) {

        Id *id = read_id(fc, true, true, true);
        Idf *idf = idf_by_id(fc, scope, id, true);

        if (idf && idf->type == idf_class) {
            Class *class = idf->item;
            result = class->is_rc;
        } else {
            sprintf(fc->sbuf, "Not a class or type: '%s'", id->name);
            fc_error(fc);
        }

    } else if (strcmp(token, "IS_NULLABLE_TYPE") == 0) {

        Id *id = read_id(fc, true, true, true);
        Idf *idf = idf_by_id(fc, scope, id, true);

        // if (idf && idf->type == idf_type) {
        //     Type *type = idf->item;
        //     result = type->nullable;
        // } else {
        sprintf(fc->sbuf, "Not a type: '%s'", id->name);
        fc_error(fc);
        // }

    } else if (strcmp(token, "IS_SIGNED_NUMBER_TYPE") == 0) {

        Id *id = read_id(fc, true, true, true);
        Idf *idf = idf_by_id(fc, scope, id, true);

        if (idf && idf->type == idf_class) {
            Class *class = idf->item;
            result = class->is_signed;
        } else {
            sprintf(fc->sbuf, "Not a type/class: '%s'", id->name);
            fc_error(fc);
        }

    } else if (strcmp(token, "IS_UNSIGNED_NUMBER_TYPE") == 0) {

        Id *id = read_id(fc, true, true, true);
        Idf *idf = idf_by_id(fc, scope, id, true);

        if (idf && idf->type == idf_class) {
            Class *class = idf->item;
            result = !class->is_signed;
        } else {
            sprintf(fc->sbuf, "Not a type/class: '%s'", id->name);
            fc_error(fc);
        }

    } else if (strcmp(token, "TYPE_HAS_PROP") == 0) {

        Id *id = read_id(fc, true, true, true);
        Idf *idf = idf_by_id(fc, scope, id, true);

        Class *class = NULL;
        if (idf && idf->type == idf_class) {
            class = idf->item;
        }
        if (!class) {
            sprintf(fc->sbuf, "Not a type/class: '%s'", id->name);
            fc_error(fc);
        }

        tok(fc, token, true, true);

        result = map_get(class->props, token) ? true : false;

    } else if (strcmp(token, "TYPE_HAS_FUNC") == 0) {

        Id *id = read_id(fc, true, true, true);
        Idf *idf = idf_by_id(fc, scope, id, true);

        Class *class = NULL;
        if (idf && idf->type == idf_class) {
            class = idf->item;
        }
        if (!class) {
            sprintf(fc->sbuf, "Not a type/class: '%s'", id->name);
            fc_error(fc);
        }

        tok(fc, token, true, true);

        result = map_get(class->funcs, token) ? true : false;

    } else if (strcmp(token, "TYPE_IS_VOID") == 0) {

        Id *id = read_id(fc, true, true, true);
        Idf *idf = idf_by_id(fc, scope, id, true);

        // if (idf && idf->type == idf_type) {
        //     Type *type = idf->item;
        //     result = type == NULL;
        // } else {
        sprintf(fc->sbuf, "Not a type: '%s'", id->name);
        fc_error(fc);
        // }

    } else {

        if (!is_valid_varname(token)) {
            sprintf(fc->sbuf, "Invalid macro variable name: '%s'", token);
            fc_error(fc);
        }

        // Check if var exists and equals to "1"
        char *value = macro_get_var(mc, token);
        if (!value) {
            sprintf(fc->sbuf, "Unknown macro value: '%s'", token);
            fc_error(fc);
        }
        result = strcmp(value, "1") == 0;

        tok(fc, token, true, true);
        if (strcmp(token, "=") == 0) {
            tok(fc, token, true, true);
            if (result) {
                result = strcmp(value, token) == 0;
            }
        } else {
            rtok(fc);
        }
    }

    tok(fc, token, true, true);
    if (strcmp(token, "&&") == 0) {
        result = result && macro_resolve_if_value(fc, scope, mc);
    } else if (strcmp(token, "||") == 0) {
        bool next = macro_resolve_if_value(fc, scope, mc);
        result = result || next;
    } else {
        rtok(fc);
    }

    return result;
}

char *macro_get_var(MacroScope *mc, char *key) {
    //
    while (mc) {
        char *v = map_get(mc->identifiers, key);
        if (v) {
            return v;
        }
        mc = mc->parent;
    }
    return NULL;
}
