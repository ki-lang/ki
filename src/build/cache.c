
#include "../all.h"
#include "../libs/md5.h"

// After reading all types
// - Generate all hashes
// - Loop fc's and check if hashes hash changed
// -- if yes: set recompile and clear depends_on

FcCache *init_fc_cache() {
    //
    FcCache *c = malloc(sizeof(FcCache));
    c->modified_time = 0;
    c->depends_on = map_make();
    c->allocators = map_make();

    return c;
}

void free_fc_cache(FcCache *c) {
    //
    map_free(c->depends_on, true);
    free(c);
}

void fc_load_cache(FileCompiler *fc) {
    //
    FcCache *c = init_fc_cache();

    if (file_exists(fc->cache_filepath)) {
        Str *content_str = file_get_contents(fc->cache_filepath);
        char *content = str_to_chars(content_str);
        // const nx_json *json = nx_json_parse(content, 0);
        cJSON *json = cJSON_ParseWithLength(content, content_str->length);
        free(content_str);

        if (json != NULL) {
            const cJSON *mt = cJSON_GetObjectItemCaseSensitive(json, "modified_time");
            if (mt != NULL) {
                c->modified_time = mt->valueint;
            }
            //
            const cJSON *deps = cJSON_GetObjectItemCaseSensitive(json, "depends_on");
            if (deps != NULL) {
                cJSON *item = deps->child;
                while (item) {
                    char *key = item->string;
                    char *value = item->valuestring;

                    map_set(c->depends_on, strdup(key), strdup(value));

                    item = item->next;
                }
            }
            //
            const cJSON *alcs = cJSON_GetObjectItemCaseSensitive(json, "allocators");
            if (alcs != NULL) {
                cJSON *item = alcs->child;
                while (item) {
                    char *key = item->string;
                    char *value = item->valuestring;

                    map_set(c->allocators, strdup(key), strdup(value));

                    item = item->next;
                }
            }
        }

        cJSON_Delete(json);
    }

    fc->cache = c;
}

void fc_save_cache(FileCompiler *fc) {
    //
    FcCache *c = fc->cache;
    cJSON *json = cJSON_CreateObject();

    cJSON *mt = cJSON_CreateNumber(c->modified_time);

    cJSON_AddItemToObject(json, "modified_time", mt);

    //
    cJSON *depends_on = cJSON_CreateObject();
    for (int i = 0; i < c->depends_on->keys->length; i++) {
        char *kipath = array_get_index(c->depends_on->keys, i);
        char *modtime = array_get_index(c->depends_on->values, i);

        cJSON *val = cJSON_CreateString(modtime);
        cJSON_AddItemToObject(depends_on, kipath, val);
    }
    cJSON_AddItemToObject(json, "depends_on", depends_on);

    //
    cJSON *allocators = cJSON_CreateObject();
    for (int i = 0; i < c->allocators->keys->length; i++) {
        char *size = array_get_index(c->allocators->keys, i);
        char *threaded = array_get_index(c->allocators->values, i);

        cJSON *val = cJSON_CreateString(threaded);
        cJSON_AddItemToObject(allocators, size, val);
    }
    cJSON_AddItemToObject(json, "allocators", allocators);

    // Write
    char *content = cJSON_Print(json);
    cJSON_Delete(json);

    write_file(fc->cache_filepath, content, false);
}

void fc_check_if_modified(FileCompiler *fc) {
    //
    Map *depends_on = fc->cache->depends_on;
    for (int i = 0; i < depends_on->keys->length; i++) {
        char *kipath = array_get_index(depends_on->keys, i);
        char *modtime = array_get_index(depends_on->values, i);

        FileCompiler *depfc = map_get(g_fc_by_ki_filepath, kipath);
        if (!depfc) {
            fc->should_recompile = true;
            fc->cache->depends_on = map_make();
            break;
        }
        sprintf(fc->sprintf, "%d", depfc->cache->modified_time);
        if (strcmp(modtime, fc->sprintf) != 0) {
            fc->should_recompile = true;
            fc->cache->depends_on = map_make();
            break;
        }
    }
}

void fc_depends_on(FileCompiler *fc, FileCompiler *depfc) {
    //
    if (fc == depfc)
        return;

    if (fc->should_recompile) {
        Map *depends_on = fc->cache->depends_on;
        char *mod_time = map_get(depends_on, depfc->ki_filepath);
        if (!mod_time) {
            sprintf(fc->sprintf, "%d", depfc->cache->modified_time);
            map_set(depends_on, depfc->ki_filepath, strdup(fc->sprintf));
        }
    }
}
