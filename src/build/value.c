
#include "../all.h"

Value *value_handle_idf(Fc *fc, Allocator *alc, Scope *scope, Id *id, Idf *idf);

Value *value_init(Allocator *alc, int type, void *item, Type *rett) {
    //
    Value *v = al(alc, sizeof(Value));
    v->type = type;
    v->item = item;
    v->rett = rett;

    return v;
}

Value *read_value(Fc *fc, Allocator *alc, Scope *scope, bool sameline, int prio) {
    //
    char *token = fc->token;
    Build *b = fc->b;
    Value *v = NULL;

    tok(fc, token, sameline, true);

    if (strcmp(token, "(") == 0) {
        v = read_value(fc, alc, scope, false, 0);
        //
    } else if (strcmp(token, "\"") == 0) {
        Str *str = read_string(fc);
        char *body = str_to_chars(alc, str);
        v = value_init(alc, v_string, body, type_gen(fc->b, alc, "String"));
        //
    } else if (strcmp(token, "ptrv") == 0) {
        Value *on = read_value(fc, alc, scope, false, 0);
        tok_expect(fc, "as", true, true);
        Type *type = read_type(fc, alc, scope, true, true);
        v = vgen_ptrv(alc, on, type);
        //
    } else if (is_valid_varname_char(token[0])) {
        rtok(fc);
        Id *id = read_id(fc, sameline, true, true);
        Idf *idf = idf_by_id(fc, scope, id, true);
        v = value_handle_idf(fc, alc, scope, id, idf);
    } else {
        sprintf(fc->sbuf, "Unknown value: '%s'", token);
        fc_error(fc);
    }

    if (!v) {
        sprintf(fc->sbuf, "Missing value (bug)");
        fc_error(fc);
    }

    return v;
}

Value *value_handle_idf(Fc *fc, Allocator *alc, Scope *scope, Id *id, Idf *idf) {
    //
    if (idf->type == idf_var) {
        Var *var = idf->item;
        return value_init(alc, v_var, idf->item, var->type);
    }

    if (idf->type == idf_func) {
        Func *func = idf->item;
    }

    if (idf->type == idf_class) {
        Class *class = idf->item;
    }

    sprintf(fc->sbuf, "Cannot convert identifier to a value: '%s'", id->name);
    fc_error(fc);
}

Value *try_convert(Fc *fc, Allocator *alc, Value *val, Type *to_type) {
    //
}