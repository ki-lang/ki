
#include "../all.h"

char *loader_find_config_dir(Build *b, char *dir) {
    //
    Allocator *alc = b->alc;

    dir = dups(alc, dir);
    char cfg_path[strlen(dir) + 10];

    while (strlen(dir) > 3) {
        strcpy(cfg_path, dir);
        strcat(cfg_path, "ki.json");
        if (file_exists(cfg_path)) {
            return dir;
        }
        get_dir_from_path(dir, dir);
    }
    return NULL;
}

Pkc *loader_find_pkc(Build *b, char *dir) {
    //
    // printf("find pkc: %s\n", dir);
    Pkc *pkc = map_get(b->packages_by_dir, dir);
    if (pkc) {
        return pkc;
    }

    Allocator *alc = b->alc;
    char *name = NULL;

    char *cfg_dir = loader_find_config_dir(b, dir);
    if (cfg_dir) {
        pkc = map_get(b->packages_by_dir, cfg_dir);
        if (pkc) {
            return pkc;
        }
    }
    if (!cfg_dir) {
        cfg_dir = b->main_dir;
        pkc = map_get(b->packages_by_dir, cfg_dir);
        if (pkc) {
            return pkc;
        }
    }

    // printf("cfg_dir: %s\n", cfg_dir);
    Config *cfg = cfg_load(alc, b->str_buf, cfg_dir);
    if (cfg) {
        cJSON *name_ = cJSON_GetObjectItemCaseSensitive(cfg->json, "name");
        if (name_ && name_->valuestring) {
            name = dups(alc, name_->valuestring);
        }
    }

    bool is_main = false;
    if (!name) {
        if (b->pkc_main) {
            name = al(alc, 64);
            simple_hash(cfg->dir, name);
        } else {
            is_main = true;
            name = "main";
        }
    }
    // printf("name: %s\n", name);

    pkc = pkc_init(alc, b, name, cfg_dir, cfg);
    map_set(b->packages_by_dir, dir, pkc);
    map_set(b->packages_by_dir, cfg_dir, pkc);

    if (is_main) {
        b->pkc_main = pkc;
        b->nsc_main = nsc_init(alc, b, pkc, "main");
    }

    return pkc;
}

Pkc *loader_find_pkc_for_file(Build *b, char *path) {
    char dir[KI_PATH_MAX];
    get_dir_from_path(path, dir);
    return loader_find_pkc(b, dir);
}

Nsc *loader_get_nsc_for_file(Pkc *pkc, char *path) {
    char dir[KI_PATH_MAX];
    get_dir_from_path(path, dir);
    return loader_get_nsc_for_dir(pkc, dir);
}

Nsc *loader_get_nsc_for_dir(Pkc *pkc, char *dir) {
    //
    // printf("# Find nsc for: %s\n", dir);

    Build *b = pkc->b;
    Allocator *alc = b->alc;
    Nsc *nsc = NULL;
    char *name = "main";

    // Find by dir
    nsc = map_get(b->namespaces_by_dir, dir);
    if (nsc) {
        return nsc;
    }

    if (pkc->config) {
        cJSON *json = pkc->config->json;
        cJSON *namespaces = cJSON_GetObjectItemCaseSensitive(json, "namespaces");
        if (namespaces) {
            cJSON *ns = namespaces->child;
            while (ns) {
                char ndir[KI_PATH_MAX];
                strcpy(ndir, pkc->dir);
                strcat(ndir, "/");
                strcat(ndir, ns->valuestring);

                char dir_abs[KI_PATH_MAX];
                get_fullpath(ndir, dir_abs);

                if (strcmp(dir, dir_abs) != 0) {
                    ns = ns->next;
                    continue;
                }

                // Name found
                name = dups(alc, ns->string);
                break;
            }
        }
    }

    nsc = map_get(pkc->namespaces, name);
    if (nsc) {
        return nsc;
    }

    // Create new namespace
    nsc = nsc_init(alc, b, pkc, name);
    map_set(b->namespaces_by_dir, dir, nsc);

    return nsc;
}

Nsc *loader_load_nsc(Pkc *pkc, char *name) {
    //
    Nsc *nsc = map_get(pkc->namespaces, name);
    if (nsc)
        return nsc;

    if (!pkc->config) {
        return NULL;
    }

    Build *b = pkc->b;
    Allocator *alc = b->alc;

    cJSON *json = pkc->config->json;
    cJSON *namespaces = cJSON_GetObjectItemCaseSensitive(json, "namespaces");
    if (!namespaces) {
        return NULL;
    }

    cJSON *ns = cJSON_GetObjectItemCaseSensitive(namespaces, name);
    if (!ns)
        return NULL;

    char ndir[KI_PATH_MAX];
    strcpy(ndir, pkc->dir);
    strcat(ndir, ns->valuestring);
    char lch = ndir[strlen(ndir) - 1];
    if (lch != '/' && lch != '\\')
        strcat(ndir, "/");

    char dir_abs[KI_PATH_MAX];
    get_fullpath(ndir, dir_abs);

    if (!dir_exists(dir_abs)) {
        sprintf(b->sbuf, "Namespace directory not found: '%s'", dir_abs);
        die(b->sbuf);
    }

    //
    Pkc *rpkc = loader_find_pkc(b, dir_abs);
    nsc = loader_get_nsc_for_dir(rpkc, dir_abs);
    map_set(pkc->namespaces, name, nsc);

    // Name found
    Array *files = get_subfiles(alc, dir_abs, false, true);
    int len = files->length;
    int i = 0;
    while (i < len) {
        char *fn = array_get_index(files, i);
        i++;
        if (!ends_with(fn, ".ki")) {
            continue;
        }

        char *path = al(alc, KI_PATH_MAX);
        strcpy(path, dir_abs);
        strcat(path, fn);
        fc_init(b, path, false);
    }

    return nsc;
}
