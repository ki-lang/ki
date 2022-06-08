
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

    // Scan globals
    for (int i = 0; i < packages->keys->length; i++) {
        PkgCompiler *pkc = array_get_index(packages->values, i);
        for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
            FileCompiler *fc = array_get_index(pkc->file_compilers->values, o);
            fc_scan_globals(fc);
        }
    }
}

void fc_scan_args_and_props(FileCompiler *fc) {
    //
    for (int x = 0; x < fc->functions->length; x++) {
        Function *func = array_get_index(fc->functions, x);
        fc_scan_func_args(func);
    }
}

void fc_scan_globals(FileCompiler *fc) {
    //
    for (int x = 0; x < fc->globals->length; x++) {
        GlobalVar *gv = array_get_index(fc->globals, x);

        fc->i = gv->fc_i;

        gv->return_type = fc_read_type(fc, fc->scope);

        IdentifierFor *idf = init_idf();
        idf->type = gv->type == gv_threaded ? idfor_threaded_global : idfor_shared_global;
        idf->item = gv;

        Scope *scope = fc->nsc->scope;
        map_set(scope->identifiers, gv->name, idf);
        map_set(c_identifiers, gv->cname, idf);

        if (gv->return_type->is_pointer && !gv->return_type->nullable) {
            fc_error(fc, "Global variables must be nullable (null is their default value)", NULL);
        }
    }
}
