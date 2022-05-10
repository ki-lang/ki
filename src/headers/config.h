
typedef struct Config {
    char *path;
    char *content;
    cJSON *json;
} Config;

Config *cfg_get(char *dir);
bool cfg_has_package(Config *cfg, char *name);
void cfg_save(Config *cfg);
