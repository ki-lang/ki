
#include "all.h"

void cmd_build(Array* files, Map* options) {
  LOC = 0;
  double parse_time, write_c_time, compile_time;
  parse_time = get_time();

  printf("Build...\n");
  //
  cmd_build_init_static();
  // Globals
  packages = map_make();
  headers = map_make();
  o_files = array_make(10);
  cmd_arg_files = array_make(2);
  c_identifiers = map_make();
  c_struct_identifiers = map_make();
  c_enum_identifiers = map_make();
  c_union_identifiers = map_make();
  allow_new_namespaces = true;
  last_readonly_i = 0;
  GEN_C = 0;
  //
  PkgCompiler* pkc = init_pkc();
  pkc->name = "main";
  map_set(packages, pkc->name, pkc);

  pkc_create_namespace(pkc, "main");

  // Step 1. create fc and scan types
  // Step 2. scan headers
  printf("# SCAN TYPES\n");
  for (int i = 0; i < files->length; i++) {
    char* filepath = array_get_index(files, i);
    fc_new_file(pkc, filepath, true);
  }

  allow_new_namespaces = false;

  // Step 3. Scan values
  printf("# SCAN ARGS/PROPS\n");
  fc_scan_values();

  // Step 4. Build ASTs
  printf("# BUILD AST\n");
  fc_build_asts();
  parse_time = get_time() - parse_time;

  // Step 5. Write c
  printf("# TRANSLATE AST\n");
  write_c_time = get_time();
  fc_write_c_all();
  write_c_time = get_time() - write_c_time;

  // Step 6. Compile
  printf("# COMPILE\n");
  compile_time = get_time();
  compile_all();
  compile_time = get_time() - compile_time;

  // Free variables

  //
  printf("\n");
  printf("Parse time: %f\n", parse_time);
  printf("Write-c time: %f\n", write_c_time);
  printf("Compile time: %f\n", compile_time);
  printf("Lines of code: %d\n", LOC);
  printf("\n");
}

void cmd_build_init_static() {
  //
  if (internal_types != NULL) {
    return;
  }

  pointer_size = 8;

  internal_types = array_make(10);
  array_push(internal_types, "i8");
  array_push(internal_types, "u8");
  array_push(internal_types, "i32");
  array_push(internal_types, "u32");
  array_push(internal_types, "ptr");
  array_push(internal_types, "bool");
  array_push(internal_types, "string");
}

#ifdef WIN32

double get_time() {
  LARGE_INTEGER t, f;
  QueryPerformanceCounter(&t);
  QueryPerformanceFrequency(&f);
  return (double)t.QuadPart / (double)f.QuadPart;
}

#else

double get_time() {
  struct timeval t;
  struct timezone tzp;
  gettimeofday(&t, &tzp);
  return t.tv_sec + t.tv_usec * 1e-6;
}

#endif
