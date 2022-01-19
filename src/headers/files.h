
#include "../all.h"

char *get_storage_path();
char* get_cache_dir();
char* get_binary_dir();

Array* get_subfiles(char*, bool, bool);
char *get_fullpath(char *filepath);

int file_exists(const char* path);
int dir_exists(const char* path);
void makedir(char*, int);
char *get_dir_from_path(char *path);
char *get_path_basename(char *path);
char *strip_ext(char *fn);

void write_file(char* file, char* content, bool append);
Str *file_get_contents(char *path);
