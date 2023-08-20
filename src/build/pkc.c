
#include "../all.h"

void pkc_load_config(Pkc *pkc);

Pkc *pkc_init(Allocator *alc, Build *b, char *name, char *dir, Config *cfg) {
    //
    Pkc *pkc = al(alc, sizeof(Pkc));
    pkc->b = b;
    pkc->main_nsc = NULL;
    pkc->sub_packages = map_make(alc);
    pkc->namespaces = map_make(alc);
    pkc->name = name;
    pkc->dir = dir;
    pkc->hash = al(alc, 64);
    pkc->config = NULL;
    pkc->header_dirs = array_make(alc, 10);
    pkc->config = cfg;

    simple_hash(pkc->dir, pkc->hash);

    array_push(b->packages, pkc);

    pkc_load_config(pkc);

    // if (!name) {
    //     // Load name from package config
    //     if (pkc->config) {
    //         cJSON *json = pkc->config->json;
    //         const cJSON *jname = cJSON_GetObjectItemCaseSensitive(json, "name");
    //         pkc->name = dups(alc, jname->valuestring);
    //     }
    //     if (!pkc->name) {
    //         sprintf(b->sbuf, "Package in directory '%s' has no name defined", dir);
    //         die(b->sbuf);
    //     }
    // }

    return pkc;
}

Nsc *pkc_get_nsc(Pkc *pkc, char *name) {
    //
    Nsc *nsc = map_get(pkc->namespaces, name);
    if (!nsc) {
        sprintf(pkc->b->sbuf, "Namespace not found: '%s'", name);
        build_error(pkc->b, pkc->b->sbuf);
    }
    return nsc;
}

void pkc_load_config(Pkc *pkc) {

    Build *b = pkc->b;
    Allocator *alc = b->alc;
    char *dir = pkc->dir;

    Config *cfg = pkc->config;
    // Config *cfg = cfg_load(alc, b->str_buf, dir);
    if (!cfg)
        return;

    pkc->config = cfg;

    cJSON *headers = cJSON_GetObjectItemCaseSensitive(cfg->json, "headers");
    if (headers) {
        cJSON *dirs = cJSON_GetObjectItemCaseSensitive(headers, "dirs");
        if (dirs) {
            char fullpath[KI_PATH_MAX];
            cJSON *cdir = dirs->child;
            while (cdir) {

                strcpy(fullpath, pkc->dir);
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

Pkc *pkc_get_sub_package(Pkc *pkc, char *name) {
    //
    if (strcmp(name, "ki") == 0) {
        return pkc->b->pkc_ki;
    }

    Pkc *res = map_get(pkc->sub_packages, name);
    if (res) {
        return res;
    }

    Build *b = pkc->b;

    // Load from config
    Config *cfg = pkc->config;
    if (!cfg)
        return NULL;
    cJSON *json = cfg->json;
    if (cfg_has_package(cfg, name)) {
        const cJSON *pkgs = cJSON_GetObjectItemCaseSensitive(json, "packages");
        const cJSON *item = cJSON_GetObjectItemCaseSensitive(pkgs, name);
        const cJSON *jname = cJSON_GetObjectItemCaseSensitive(item, "name");
        const cJSON *jversion = cJSON_GetObjectItemCaseSensitive(item, "version");

        if (!jname || !jname->valuestring || !jversion || !jversion->valuestring) {
            sprintf(b->sbuf, "Package '%s' has no name/version defined in: %s", name, cfg->path);
            build_error(b, b->sbuf);
        }

        char pkgpath[KI_PATH_MAX];
        char versionpath[KI_PATH_MAX];
        char config_path[KI_PATH_MAX];

        pkg_get_dir(b->pkg_dir, jname->valuestring, pkgpath);

        strcpy(versionpath, pkgpath);
        strcat(versionpath, "/");
        strcat(versionpath, jversion->valuestring);
        strcat(versionpath, "/");

        strcpy(config_path, versionpath);
        strcat(config_path, "ki.json");

        if (!file_exists(config_path)) {
            sprintf(b->sbuf, "Package '%s' has no 'ki.json' config. Expected file: '%s'", name, config_path);
            build_error(b, b->sbuf);
        }

        Pkc *sub = loader_get_pkc_for_dir(b, versionpath);

        map_set(pkc->sub_packages, name, sub);

        return sub;
    }

    return NULL;
}
