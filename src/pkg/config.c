
#include "../all.h"

Config *cfg_load(Allocator *alc, Str *buf, char *dir) {
    //
    char path[strlen(dir) + 20];
    strcpy(path, dir);
    strcat(path, "/ki.json");

    if (!file_exists(path)) {
        return NULL;
    }

    char *cpath = dups(alc, path);

    file_get_contents(buf, cpath);
    char *content = str_to_chars(alc, buf);

    cJSON *json = cJSON_ParseWithLength(content, buf->length);

    Config *cfg = malloc(sizeof(Config));
    cfg->path = cpath;
    cfg->dir = dir;
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
    free(content);
}
