
#include "../all.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#else
pid_t child_pid, wpid;
int status = 0;
#endif

void compile_all() {
  if (o_files->length == 0) {
    printf("Nothing to compile\n");
    return;
  }

  //
  for (int i = 0; i < packages->keys->length; i++) {
    PkgCompiler* pkc = array_get_index(packages->values, i);
    for (int o = 0; o < pkc->file_compilers->keys->length; o++) {
      FileCompiler* fc = array_get_index(pkc->file_compilers->values, o);
      if (fc->create_o_file) {
        fc_compile_o_file(fc);
      }
    }
  }

  //
  wait_cmd();

  // Compile executable
  char* cmd = malloc(3000);
  strcpy(cmd, get_compiler_path());
  strcat(cmd, " -o test");

  for (int i = 0; i < o_files->length; i++) {
    strcat(cmd, " ");
    strcat(cmd, array_get_index(o_files, i));
  }

  int result = run_cmd(cmd);
  if (result == -1) {
    printf("Compile failed\n");
    exit(1);
  }

  free(cmd);
}

void fc_compile_o_file(FileCompiler* fc) {
  //
  char* cmd = malloc(3000);
  strcpy(cmd, get_compiler_path());
  strcat(cmd, " -g -O0 -c");
  strcat(cmd, " -I ");
  strcat(cmd, get_binary_dir());
  strcat(cmd, " -o ");
  strcat(cmd, fc->o_filepath);
  strcat(cmd, " ");
  strcat(cmd, fc->c_filepath);

  printf("Write .o: %s\n", fc->o_filepath);

  int result = run_cmd(cmd);
  if (result == -1) {
    printf("Compile .o failed\n");
    exit(1);
  }

  free(cmd);
}

////////

char* get_compiler_path() {
  char* cmd = malloc(1000);
  strcpy(cmd, "gcc");
  return cmd;
  strcpy(cmd, get_binary_dir());
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  strcat(cmd, "/misc/tcc/win/tcc.exe");
#else
  strcat(cmd, "/misc/tcc/linux/tcc");
#endif
  return cmd;
}

int run_cmd(char* cmd) {
  int result = 0;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  result = system(cmd);
#else
  child_pid = fork();
  if (child_pid == -1) {
    perror("fork");
  } else if (child_pid == 0) {
    execlp("/bin/sh", "/bin/sh", "-c", cmd, NULL);
    // system(cmd);
  }
#endif
  return result;
}

void wait_cmd() {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#else
  while ((wpid = wait(&status)) > 0)
    ;
#endif
}