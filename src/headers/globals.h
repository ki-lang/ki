
#include "../all.h"

Map* packages;
Map* headers;
Array* o_files;
Array* cmd_arg_files;

CImport* c_importer;
Map* c_identifiers;
Map* c_struct_identifiers;
Map* c_enum_identifiers;
Map* c_union_identifiers;

bool allow_new_namespaces;
bool disable_macro_check;

Array* internal_types;

int pointer_size;

//

int last_readonly_i;
int LOC;

// Write c
int GEN_C;
