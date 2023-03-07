
void die(char *msg);
void parse_argv(char **argv, int argc, Array *has_value, Array *args, Map *options);
bool ends_with(const char *str, const char *suffix);
char *rand_string(char *str, int size);
int atoi(const char *str);

// Alloc
void *b_alloc(Build *b, size_t size);
void *fc_alloc(Fc *fc, size_t size);

// Build
void cmd_build(int argc, char **argv);

// Fc
Fc *fc_init(Build *b, char *path_ki);
