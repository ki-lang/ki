
#include "../all.h"

Map* packages;
Array* headers;
Array* o_files;
Array* cmd_arg_files;
Map* allocators;
Map* macro_defines;

Map* c_identifiers;
Map* c_struct_identifiers;
Map* c_enum_identifiers;
Map* c_union_identifiers;

bool allow_new_namespaces;
bool uses_async;

Array* internal_types;

int pointer_size;

//

int last_readonly_i;
int LOC;

// Write c
int GEN_C;
