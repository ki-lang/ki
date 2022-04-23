
#include "../all.h"

// Before we build an AST we need to know the class
// properties, function args and other globals

// Scan:
// Global vars
// Class props
// Function args & return type

void fc_scan_values() {
    //
    for (int i = 0; i < packages->keys->length; i++) {
        PkgCompiler *pkc = array_get_index(packages->values, i);

        for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
            FileCompiler *fc = array_get_index(pkc->file_compilers->values, o);
            fc_scan_args_and_props(fc);
            fc_scan_threaded_globals(fc);
        }
    }

    // Scan class props
    for (int x = 0; x < c_identifiers->keys->length; x++) {
        IdentifierFor *idf = array_get_index(c_identifiers->values, x);
        if (idf->type == idfor_class) {
            Class *class = idf->item;
            if (class->self_scan)
                continue;
            fc_scan_class_props(class);
            // printf("%s\n", class->cname);
            // map_print_keys(class->props);
        }
    }
    // Scan class prop values
    for (int x = 0; x < c_identifiers->keys->length; x++) {
        IdentifierFor *idf = array_get_index(c_identifiers->values, x);
        if (idf->type == idfor_class) {
            Class *class = idf->item;
            if (class->self_scan)
                continue;
            fc_scan_class_prop_values(class);
        }
    }
}

void fc_scan_args_and_props(FileCompiler *fc) {
    // Uses
    for (int x = 0; x < fc->uses->keys->length; x++) {
        char *name = array_get_index(fc->uses->keys, x);
        FcUse *fcu = array_get_index(fc->uses->values, x);

        IdentifierFor *idf = map_get(fcu->nsc->scope->identifiers, name);

        if (!idf) {
            fc_error(fc, "Not found: '%s'\n", name);
        }

        map_set(fc->scope->identifiers, name, idf);
    }

    //
    for (int x = 0; x < fc->functions->length; x++) {
        Function *func = array_get_index(fc->functions, x);
        fc_scan_func_args(func);
    }
}

void fc_scan_threaded_globals(FileCompiler *fc) {
    char *token = malloc(KI_TOKEN_MAX);

    for (int i = 0; i < fc->threaded_globals->length; i++) {
        ThreadedGlobal *tg = array_get_index(fc->threaded_globals, i);
        fc->i = tg->i;
        tg->type = fc_read_type(fc, fc->scope);
        fc_next_token(fc, token, false, true, true);

        // Name
        char *name = strdup(token);
        IdentifierFor *idf = map_get(fc->nsc->scope->identifiers, name);
        if (idf) {
            fc_error(fc, "Name already used: %s", name);
        }

        tg->name = name;
        fc_expect_token(fc, "=", false, true, true);
        // Value
        tg->default_value = fc_read_value(fc, fc->nsc->scope, false, true, true);
        //
        fc_expect_token(fc, ";", false, true, true);

        // Set identifier
        idf = init_idf();
        idf->type = idfor_threaded_var;
        idf->item = tg;
        map_set(fc->nsc->scope->identifiers, name, idf);
    }

    free(token);
}
