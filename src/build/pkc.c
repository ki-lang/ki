
#include "../all.h"

void pkc_load_config(Pkc *pkc);
void pkc_load_macros(Pkc *pkc);

Pkc *pkc_init(Allocator *alc, Build *b, char *name, char *dir) {
    //
    if (!file_exists(dir)) {
        sprintf(b->sbuf, "Package directory for '%s' not found. Directory: '%s'", name, dir);
        die(b->sbuf);
    }

    Pkc *pkc = al(alc, sizeof(Pkc));
    pkc->b = b;
    pkc->sub_packages = map_make(alc);
    pkc->namespaces = map_make(alc);
    pkc->name = name;
    pkc->dir = dir;
    pkc->hash = al(alc, 64);
    pkc->macro_file = NULL;
    pkc->config = NULL;
    pkc->header_dirs = array_make(alc, 10);

    simple_hash(dir, pkc->hash);

    pkc_load_macros(pkc);
    pkc_load_config(pkc);

    if (!name) {
        // Load name from package config
        if (pkc->config) {
            cJSON *json = pkc->config->json;
            const cJSON *jname = cJSON_GetObjectItemCaseSensitive(json, "name");
            pkc->name = dups(alc, jname->valuestring);
        }
        if (!pkc->name) {
            sprintf(b->sbuf, "Package in directory '%s' has no name defined", dir);
            die(b->sbuf);
        }
    }

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
                        fc_init(b, path, nsc, nsc->pkc, false);
                    }
                }

                return nsc;
            }
        }
    }

    if (parsing_fc) {
        sprintf(parsing_fc->sbuf, "Cannot find namespace '%s' in package '%s'", name, pkc->name);
        fc_error(parsing_fc);
    } else {
        sprintf(pkc->b->sbuf, "Cannot find namespace '%s' in package '%s'", name, pkc->name);
        die(pkc->b->sbuf);
    }
    return NULL;
}

void pkc_load_macros(Pkc *pkc) {
    //
    char path[KI_PATH_MAX];
    strcpy(path, pkc->dir);
    strcat(path, "/macro.ki");
    if (file_exists(path)) {
        pkc->macro_file = dups(pkc->b->alc, path);
    }
}

void pkc_load_config(Pkc *pkc) {

    Build *b = pkc->b;
    Allocator *alc = b->alc;
    char *dir = pkc->dir;

    Config *cfg = cfg_load(alc, b->str_buf, dir);
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

Pkc *pkc_get_sub_package(Pkc *pkc, char *name) {
    //
    if (strcmp(name, "ki") == 0) {
        return pkc->b->pkc_ki;
    }

    Pkc *res = map_get(pkc->sub_packages, name);
    if (res) {
        return res;
    }

    char msg[256];
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
            sprintf(msg, "Package '%s' has no name/version defined in: %s", name, cfg->path);
            die(msg);
        }

        char pkgpath[KI_PATH_MAX];
        char versionpath[KI_PATH_MAX];

        pkg_get_dir(b->pkg_dir, jname->valuestring, pkgpath);

        strcpy(versionpath, pkgpath);
        strcat(versionpath, "/");
        strcat(versionpath, jversion->valuestring);

        Pkc *sub = map_get(b->packages_by_dir, versionpath);
        if (!sub) {
            sub = pkc_init(b->alc, b, NULL, versionpath);
        }

        map_set(pkc->sub_packages, name, sub);
        array_push(b->packages, sub);

        return sub;
    }

    return NULL;
}
