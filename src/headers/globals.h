
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
//
Function *g_main_func;
Map *g_fc_by_ki_filepath;
Array *g_functions;
Array *g_classes;
Array *g_links;
Array *g_link_dirs;
Array *g_test_funcs;
char *g_sprintf;

// Options
char *g_arg_0;
char *g_output_name;
bool g_static;
bool g_nocache;
bool g_optimize;
bool g_verbose;
bool g_verbose_all;
bool g_run;
bool g_run_tests;

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

int cmd_err_code;

// LLVM
LLVMTargetMachineRef g_target_machine;
LLVMTargetDataRef g_target_data;

#endif
