
#include "../all.h"

void fc_scan_converters(FileCompiler *fc) {
    //
    for (int i = 0; i < fc->converter_positions->length; i++) {
        ConverterPos *cvp = array_get_index(fc->converter_positions, i);
        fc_scan_converter(fc, cvp);
    }
}

void fc_scan_converter(FileCompiler *fc, ConverterPos *cvp) {
    //
    fc->i = cvp->fc_i;
    char *token = malloc(KI_TOKEN_MAX);

    while (fc->i < fc->content_len) {
        fc_next_token(fc, token, false, false, true);

        if (token[0] == '\0') {
            fc_error(fc, "Unexpected end of file, expected a '}' to close the converter", NULL);
        }

        if (strcmp(token, "}") == 0) {
            break;
        }

        if (strcmp(token, "func") == 0) {
            fc_expect_token(fc, "(", false, true, true);

            Function *func = init_func();
            func->fc = fc;
            func->args_i = fc->i;
            func->cname = "-temp-";
            func->scope = init_sub_scope(fc->scope);
            func->scope->is_func = true;
            func->scope->func = func;

            fc_scan_func_args(func);

            if (func->args->length > 1) {
                fc_error(fc, "Too many arguments", NULL);
            }
            if (func->args->length == 0) {
                fc_error(fc, "Missing from-type argument", NULL);
            }
            if (func->can_error) {
                fc_error(fc, "Should not throw errors", NULL);
            }
            if (!func->return_type) {
                fc_error(fc, "Missing return type", NULL);
            }

            Type *from_type = array_get_index(func->arg_types, 0);
            Type *to_type = func->return_type;

            char *func_cname = malloc(KI_TOKEN_MAX);
            strcpy(func_cname, cvp->converter->cname);
            strcat(func_cname, "__");
            strcat(func_cname, from_type->class->cname);
            strcat(func_cname, "__");
            strcat(func_cname, to_type->class->cname);

            if (map_get(c_identifiers, func_cname)) {
                fc_error(fc, "A converter function for these types already exists", NULL);
            }

            func->cname = func_cname;

            array_push(g_functions, func);

            IdentifierFor *idf = init_idf();
            idf->type = idfor_func;
            idf->item = func;

            map_set(c_identifiers, func_cname, idf);

            array_push(cvp->converter->from_types, from_type->class->cname);
            array_push(cvp->converter->to_types, to_type->class->cname);
            array_push(cvp->converter->functions, func);

            continue;
        }

        fc_error(fc, "Unexpected token: '%s'", token);
    }
}

Function *converter_find_func(Converter *cv, Type *from, Type *to) {
    //
    char *from_cname = from->class->cname;
    char *to_cname = to->class->cname;
    for (int i = 0; cv->from_types->length; i++) {
        char *f = array_get_index(cv->from_types, i);
        char *t = array_get_index(cv->to_types, i);
        if (strcmp(from_cname, f) == 0 && strcmp(to_cname, t) == 0) {
            Function *func = array_get_index(cv->functions, i);
            return func;
            break;
        }
    }
    return NULL;
}
