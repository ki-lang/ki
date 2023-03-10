
#include "../all.h"

void pkc_load_config(Pkc *pkc);

Pkc *pkc_init(Allocator *alc, Build *b, char *name, char *dir) {
    //
    if (!file_exists(dir)) {
        sprintf(b->sbuf, "Package directory for '%s' not found: '%s'", name, dir);
        die(b->sbuf);
    }

    Pkc *pkc = al(alc, sizeof(Pkc));
    pkc->b = b;
    pkc->namespaces = map_make(alc);
    pkc->name = name;
    pkc->dir = dir;
    pkc->hash = al(alc, 64);
    pkc->config = NULL;

    md5(dir, pkc->hash);

    pkc_load_config(pkc);

    return pkc;
}

Nsc *pkc_get_nsc(Pkc *pkc, char *name) {
    //
    Nsc *nsc = map_get(pkc->namespaces, name);
    if (!nsc) {
        sprintf(pkc->b->sbuf, "Namespace not found: '%s'", name);
        die(pkc->b->sbuf);
    }
    return nsc;
}

Nsc *pkc_load_nsc(Pkc *pkc, char *name, Fc *parsing_fc) {
    //
    Nsc *nsc = map_get(pkc->namespaces, name);
    if (nsc) {
        return nsc;
    }

    Build *b = pkc->b;
    Allocator *alc = b->alc;

    // Check config
    if (pkc->config) {
        Config *cfg = pkc->config;
        cJSON *namespaces = cJSON_GetObjectItemCaseSensitive(cfg->json, "namespaces");
        if (namespaces) {
            cJSON *ns = cJSON_GetObjectItemCaseSensitive(namespaces, name);
            if (ns) {
                char *dir = malloc(KI_PATH_MAX);
                strcpy(dir, pkc->dir);
                strcat(dir, "/");
                strcat(dir, ns->valuestring);
                if (!file_exists(dir)) {
                    if (parsing_fc) {
                        sprintf(parsing_fc->sbuf, "Namespace directory does not exist: '%s'", dir);
                        fc_error(parsing_fc);
                    } else {
                        sprintf(b->sbuf, "Namespace directory does not exist: '%s'", dir);
                        die(b->sbuf);
                    }
                }

                nsc = nsc_init(alc, b, pkc, name);

                map_set(pkc->namespaces, nsc->name, nsc);

                Array *files = get_subfiles(alc, dir, false, true);
                int i = files->length;
                while (i > 0) {
                    i--;
                    char *path = al(alc, KI_PATH_MAX);
                    char *fn = array_get_index(files, i);
                    strcpy(path, dir);
                    strcat(path, "/");
                    strcat(path, fn);
                    if (ends_with(path, ".ki")) {
                        fc_init(b, path, nsc);
                    }
                }

                return nsc;
            }
        }
    }

    if (parsing_fc) {
        sprintf(parsing_fc->sbuf, "Cannot find namespace '%s'", name);
        fc_error(parsing_fc);
    } else {
        sprintf(pkc->b->sbuf, "Cannot find namespace '%s'", name);
        die(pkc->b->sbuf);
    }
    return NULL;
}

void pkc_load_config(Pkc *pkc) {

    char *dir = pkc->dir;
    char path[strlen(dir) + 20];
    strcpy(path, dir);
    strcat(path, "/ki.json");

    if (!file_exists(path)) {
        return;
    }

    Allocator *alc = pkc->b->alc;
    char *cpath = dups(alc, path);

    Str *content_str = file_get_contents(alc, cpath);
    char *content = str_to_chars(alc, content_str);

    cJSON *json = cJSON_ParseWithLength(content, content_str->length);

    Config *cfg = malloc(sizeof(Config));
    cfg->path = cpath;
    cfg->content = content;
    cfg->json = json;

    pkc->config = cfg;
}

void pkc_cfg_save(Config *cfg) {
    //
    char *content = cJSON_Print(cfg->json);
    write_file(cfg->path, content, false);
    free(content);
}