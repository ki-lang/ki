
#include "../all.h"

#ifdef H_GLOBALS
#else
#define H_GLOBALS 1

Map *packages;
Array *headers;
Array *o_files;
Array *cmd_arg_files;
Map *allocators;
Map *macro_defines;
char *g_output_name;
bool g_static;
Map *g_fc_by_ki_filepath;

Map *c_identifiers;
Map *c_struct_identifiers;
Map *c_enum_identifiers;
Map *c_union_identifiers;

bool allow_new_namespaces;
bool build_ast_stage;
bool uses_async;

Array *internal_types;

int pointer_size;

//

int last_readonly_i;
int LOC;

// Write c
int GEN_C;

#endif
