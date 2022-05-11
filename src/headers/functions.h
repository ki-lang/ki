
void die(char *msg);
void die_token(char *msg, char *token);
bool ends_with(const char *, const char *);
char *rand_string(char *str, int size);
int atoi(const char *str);
void prepend(char *s, const char *t);
Array *explode(char *part, char *content);
void exec_simple(char *cmd, char *output);
bool check_installed_git();
