
#include "../all.h"

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

    if (strcmp(token, "\"") == 0) {
        Str *str = read_string(fc);
        char *body = str_to_chars(alc, str);
        v = value_init(alc, v_string, body, type_gen(fc->b, alc, "String"));
    } else {
        sprintf(fc->sbuf, "Unknown value");
        fc_error(fc);
    }

    return v;
}

Value *try_convert(Fc *fc, Allocator *alc, Value *val, Type *to_type) {
    //
}