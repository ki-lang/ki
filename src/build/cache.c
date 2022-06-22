
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
    c->uses = map_make();
    c->tests_enabled = g_run_tests;

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
            const cJSON *j_uses = cJSON_GetObjectItemCaseSensitive(json, "uses");
            if (j_uses != NULL) {
                cJSON *item = j_uses->child;
                while (item) {
                    char *key = item->string;
                    cJSON *use = item->child;
                    Array *uses = array_make(2);

                    while (use) {
                        char *v = item->valuestring;
                        array_push(uses, v);
                        use = use->next;
                    }

                    map_set(c->uses, strdup(key), uses);

                    item = item->next;
                }
            }
            const cJSON *enable = cJSON_GetObjectItemCaseSensitive(json, "tests_enabled");
            if (enable != NULL) {
                c->tests_enabled = enable->valueint;
            }
        }

        cJSON_Delete(json);
    }

    fc->cache = c;

    if (c->tests_enabled != g_run_tests) {
        fc->should_recompile = true;
        fc->cache->depends_on = map_make();
    }
}

void fc_save_cache(FileCompiler *fc) {
    //
    FcCache *c = fc->cache;
    cJSON *json = cJSON_CreateObject();

    cJSON *mt = cJSON_CreateNumber(c->modified_time);
    cJSON_AddItemToObject(json, "modified_time", mt);
    cJSON *tests_enabled = cJSON_CreateNumber(c->tests_enabled);
    cJSON_AddItemToObject(json, "tests_enabled", tests_enabled);

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

    // Uses
    cJSON *j_uses = cJSON_CreateObject();
    for (int i = 0; i < c->uses->keys->length; i++) {
        char *cname = array_get_index(c->uses->keys, i);
        Array *cname_uses = array_get_index(c->uses->values, i);
        cJSON *uses = cJSON_CreateArray();

        for (int o = 0; o < cname_uses->length; o++) {
            char *cname_use = array_get_index(cname_uses, o);
            cJSON *cname_use_str = cJSON_CreateString(cname_use);
            cJSON_AddItemToArray(uses, cname_use_str);
        }

        cJSON_AddItemToObject(j_uses, cname, uses);
    }
    cJSON_AddItemToObject(json, "uses", j_uses);

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
        char *entry = map_get(depends_on, depfc->ki_filepath);
        if (!entry) {
            sprintf(fc->sprintf, "%d", depfc->cache->modified_time);
            map_set(depends_on, depfc->ki_filepath, strdup(fc->sprintf));
        }
    }
}
