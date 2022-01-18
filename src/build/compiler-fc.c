
#include "../all.h"
#include "../libs/md5.h"

FileCompiler* init_fc() {
  FileCompiler* fc = malloc(sizeof(FileCompiler));
  fc->nsc = NULL;
  //
  fc->ki_filepath = NULL;
  fc->c_filepath = NULL;
  fc->h_filepath = NULL;
  fc->o_filepath = NULL;
  fc->is_header = false;
  //
  fc->content = NULL;
  fc->content_len = 0;
  //
  fc->i = 0;
  fc->col = 0;
  fc->line = 0;
  //
  fc->macro_results = array_make(4);
  //
  fc->c_code = str_make("");
  fc->c_code_after = str_make("");
  fc->struct_code = str_make("");
  fc->h_code = str_make("");
  fc->tkn_buffer = NULL;
  fc->before_tkn_buffer = NULL;
  fc->indent = 0;
  //
  fc->scope = init_scope();
  fc->uses = map_make();
  //
  fc->create_o_file = false;
  fc->classes = array_make(2);
  fc->functions = array_make(8);
  fc->enums = array_make(4);
  return fc;
}

void free_fc(FileCompiler* fc) {
  //
  free(fc);
}

FileCompiler* fc_new_file(PkgCompiler* pkc, char* path, bool is_cmd_arg_file) {
  //
  bool is_header = ends_with(path, ".kh");
  char* ki_filepath = NULL;
  if (is_header) {
    ki_filepath = path;
  } else {
    ki_filepath = get_fullpath(path);
  }

  if (!ki_filepath) {
    printf("File not found: '%s'\n", path);
    exit(1);
  }

  if (is_cmd_arg_file) {
    array_push(cmd_arg_files, ki_filepath);
  }

  FileCompiler* fc = map_get(pkc->file_compilers, ki_filepath);
  if (fc != NULL) {
    return fc;
  }

  char* cache_dir = get_cache_dir();
  char* c_filepath = malloc(KI_PATH_MAX);
  char* h_filepath = malloc(KI_PATH_MAX);
  char* o_filepath = malloc(KI_PATH_MAX);

  char* hash = malloc(33);
  strcpy(hash, "");
  md5(ki_filepath, hash);

  strcpy(c_filepath, cache_dir);
  strcat(c_filepath, "/");
  strcat(c_filepath, hash);

  strcpy(h_filepath, c_filepath);
  strcpy(o_filepath, c_filepath);

  strcat(c_filepath, ".c");
  strcat(h_filepath, ".h");
  strcat(o_filepath, ".o");

  fc = init_fc();
  fc->nsc = pkc_create_namespace(pkc, "main");
  fc->scope->parent = fc->nsc->scope;
  //
  fc->ki_filepath = ki_filepath;
  fc->c_filepath = c_filepath;
  fc->h_filepath = h_filepath;
  fc->o_filepath = o_filepath;
  fc->is_header = is_header;
  //
  Str* content = file_get_contents(ki_filepath);
  fc->content = str_to_chars(content);
  fc->content_len = content->length;
  free(content);

  map_set(pkc->file_compilers, fc->ki_filepath, fc);

  // printf("Compile file: %s\n", fc->ki_filepath);

  fc_scan_types(fc);

  // printf("Compiled: %s\n", fc->ki_filepath);

  return fc;
}