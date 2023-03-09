
bool get_fullpath(char *filepath, char *buf);
int file_exists(const char *path);
int dir_exists(const char *path);
void get_dir_from_path(char *path, char *buf);
void filepath_pop_basename(char *path);
char *get_path_basename(Allocator *alc, char *path);
char *strip_ext(Allocator *alc, char *fn);
void makedir(char *dir, int mod);
char *get_binary_dir();
char *get_storage_path();

Array *get_subfiles(Allocator *alc, char *dir, bool dirs, bool files);
int mod_time(char *path);
void write_file(char *filepath, char *content, bool append);
Str *file_get_contents(Allocator *alc, char *path);
