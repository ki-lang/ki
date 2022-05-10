
#include "all.h"

Config *cfg_get(char *dir) {
    //
    char *path = malloc(strlen(dir) + 20);
    strcpy(path, dir);
    strcat(path, "/ki.json");

    if (!file_exists(path)) {
        free(path);
        return NULL;
    }

    Str *content_str = file_get_contents(path);
    char *content = str_to_chars(content_str);

    // const nx_json *json = nx_json_parse(content, 0);
    cJSON *json = cJSON_ParseWithLength(content, content_str->length);
    free(content_str);

    Config *cfg = malloc(sizeof(Config));
    cfg->path = path;
    cfg->content = content;
    cfg->json = json;

    return cfg;
}

bool cfg_has_package(Config *cfg, char *name) {
    //
    const cJSON *pkgs = cJSON_GetObjectItemCaseSensitive(cfg->json, "packages");
    if (pkgs == NULL) {
        return false;
    }
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(pkgs, name);
    if (item == NULL) {
        return false;
    }
    return true;
}

void cfg_save(Config *cfg) {
    //
    char *content = cJSON_Print(cfg->json);
    write_file(cfg->path, content, false);
}