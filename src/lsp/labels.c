
#include "../all.h"

char *lsp_func_insert(Allocator *alc, Func *func, char *name, bool skip_first_arg) {
    //
    char part[256];
    char type[256];
    Str *res = str_make(alc, 50);
    str_append_chars(res, name);
    str_append_chars(res, "(");
    Array *args = func->args;
    int count = 0;
    for (int i = 0; i < args->length; i++) {
        if (i == 0 && skip_first_arg) {
            continue;
        }
        if (count > 0) {
            str_append_chars(res, ", ");
        }
        Arg *arg = array_get_index(args, i);
        type_to_str(arg->type, type);

        count++;
        sprintf(part, "${%d:%s: %s}", count, arg->name, type);
        str_append_chars(res, dups(alc, part));
    }
    str_append_chars(res, ")$0");

    return str_to_chars(alc, res);
}