
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
    pkc->sub_packages = map_make(alc);
    pkc->namespaces = map_make(alc);
    pkc->name = name;
    pkc->dir = dir;
    pkc->hash = al(alc, 64);
    pkc->config = NULL;
    pkc->header_dirs = array_make(alc, 10);

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
                        fc_init(b, path, nsc, false);
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

    Str *buf = pkc->b->str_buf;
    file_get_contents(buf, cpath);
    char *content = str_to_chars(alc, buf);

    cJSON *json = cJSON_ParseWithLength(content, buf->length);

    Config *cfg = malloc(sizeof(Config));
    cfg->path = cpath;
    cfg->content = content;
    cfg->json = json;

    pkc->config = cfg;

    Build *b = pkc->b;

    cJSON *headers = cJSON_GetObjectItemCaseSensitive(cfg->json, "headers");
    if (headers) {
        cJSON *dirs = cJSON_GetObjectItemCaseSensitive(headers, "dirs");
        if (dirs) {
            char fullpath[KI_PATH_MAX];
            cJSON *cdir = dirs->child;
            while (cdir) {

                strcpy(fullpath, pkc->dir);
                strcat(fullpath, "/");
                strcat(fullpath, cdir->valuestring);

                if (!file_exists(fullpath)) {
                    printf("Header directory not found: %s => (%s)\n", cdir->valuestring, fullpath);
                    exit(1);
                }

                if (!array_contains(pkc->header_dirs, fullpath, arr_find_str)) {
                    array_push(pkc->header_dirs, dups(b->alc, fullpath));
                }

                cdir = cdir->next;
            }
        }
    }

    cJSON *link = cJSON_GetObjectItemCaseSensitive(cfg->json, "link");
    if (link) {
        cJSON *dirs = cJSON_GetObjectItemCaseSensitive(link, "dirs");
        if (dirs) {
            char fullpath[KI_PATH_MAX];
            cJSON *cdir = dirs->child;
            while (cdir) {

                strcpy(fullpath, pkc->dir);
                strcat(fullpath, "/");
                strcat(fullpath, cdir->valuestring);

                if (!file_exists(fullpath)) {
                    printf("Link directory not found: %s => (%s)\n", cdir->valuestring, fullpath);
                    exit(1);
                }

                if (!array_contains(b->link_dirs, fullpath, arr_find_str)) {
                    array_push(b->link_dirs, dups(b->alc, fullpath));
                }

                cdir = cdir->next;
            }
        }
    }
}

void pkc_cfg_save(Config *cfg) {
    //
    char *content = cJSON_Print(cfg->json);
    write_file(cfg->path, content, false);
    free(content);
}

Pkc *pkc_get_sub_package(Pkc *pkc, char *name) {
    //
    if (strcmp(name, "ki") == 0) {
        return pkc->b->pkc_ki;
    }

    Pkc *res = map_get(pkc->sub_packages, name);
    if (res) {
        return res;
    }

    // Load from config
    // TODO

    return NULL;
}
