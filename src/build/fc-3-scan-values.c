
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
    PkgCompiler* pkc = array_get_index(packages->values, i);

    for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
      FileCompiler* fc = array_get_index(pkc->file_compilers->values, o);
      fc_scan_args_and_props(fc);
    }
  }

  // Scan property values
  for (int i = 0; i < packages->keys->length; i++) {
    PkgCompiler* pkc = array_get_index(packages->values, i);
    for (int o = 0; o < pkc->namespaces->keys->length; o++) {
      NsCompiler* nsc = array_get_index(pkc->namespaces->values, o);
      fc_scan_class_prop_values(nsc);
    }
  }
}

void fc_scan_args_and_props(FileCompiler* fc) {
  // Uses
  for (int x = 0; x < fc->uses->keys->length; x++) {
    char* name = array_get_index(fc->uses->keys, x);
    FcUse* fcu = array_get_index(fc->uses->values, x);

    IdentifierFor* idf = map_get(fcu->nsc->scope->identifiers, name);

    if (!idf) {
      fc_error(fc, "Not found: '%s'\n", name);
    }

    map_set(fc->scope->identifiers, name, idf);
  }

  //
  for (int x = 0; x < fc->functions->length; x++) {
    Function* func = array_get_index(fc->functions, x);
    fc_scan_func_args(func);
  }

  for (int x = 0; x < fc->classes->length; x++) {
    Class* class = array_get_index(fc->classes, x);
    fc_scan_class_props(class);
  }
}

void fc_scan_class_prop_values(NsCompiler* nsc) {
  //
  int i = 0;
  Scope* scope = nsc->scope;
  //
  for (i = 0; i < scope->identifiers->keys->length; i++) {
    IdentifierFor* idf = array_get_index(scope->identifiers->values, i);

    if (idf->type == idfor_class) {
      Class* class = idf->item;
      FileCompiler* fc = class->fc;

      for (int o = 0; o < class->props->keys->length; o++) {
        //
        ClassProp* prop = array_get_index(class->props->values, o);
        //
        if (prop->value_i > 0) {
          fc->i = prop->value_i;
          prop->default_value = fc_read_value(fc, fc->scope, false, true, true);
          fc_expect_token(fc, ";", false, true, true);
        }
      }
    }
  }
}