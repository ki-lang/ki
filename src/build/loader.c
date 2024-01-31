
#include "../all.h"

/*
1. private func: get pkc for dir
2. public func: get nsc for dir (calls get pkc & loads all nsc files (if not main) & sets b->namespaces_by_dir[dir] = nsc)
2.0 check b->namespaces_by_dir if contains dir : if so, return its nsc
2.1 call get pkc for dir
2.2 search for correct namespace in config
2.3 if found: load all namespace files
2.4 if not found: return pkc->main_nsc (or create it if doesnt exist)
2.5 set b->namespaces_by_dir[dir] = nsc

# use pkg:ns
1. get namespace dir from pkc config
2. call get nsc for dir
*/

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

Pkc *loader_get_pkc_for_dir(Build *b, char *dir) {
    //
    Pkc *pkc = map_get(b->packages_by_dir, dir);
    if (pkc) {
        return pkc;
    }

    Allocator *alc = b->alc;
    char *name = NULL;

    char *cfg_dir = loader_find_config_dir(b, dir);
    if (!cfg_dir) {
        pkc = b->pkc_main;
        map_set(b->packages_by_dir, dir, pkc);
        return pkc;
    }

    pkc = map_get(b->packages_by_dir, cfg_dir);
    if (pkc) {
        return pkc;
    }

    unsigned long start = microtime();
    Config *cfg = cfg_load(alc, b->str_buf, cfg_dir);
    b->time_fs += microtime() - start;
    if (cfg) {
        cJSON *name_ = cJSON_GetObjectItemCaseSensitive(cfg->json, "name");
        if (name_ && name_->valuestring) {
            name = dups(alc, name_->valuestring);
        }
    }

    if (!name) {
        if (strcmp(cfg->dir, b->pkc_main->dir) == 0) {
            pkc = b->pkc_main;
        } else {
            name = al(alc, 64);
            ctxhash(cfg->dir, name);
        }
    }

    if (!pkc) {
        pkc = pkc_init(alc, b, name, cfg_dir, cfg);
    } else {
        cJSON_Delete(cfg->json);
    }
    map_set(b->packages_by_dir, dir, pkc);
    map_set(b->packages_by_dir, cfg_dir, pkc);

    return pkc;
}

Nsc *loader_get_nsc_for_dir(Build *b, char *dir) {
    //
    // 2.0 check b->namespaces_by_dir if contains dir : if so, return its nsc
    Nsc *nsc = map_get(b->namespaces_by_dir, dir);
    if (nsc) {
        return nsc;
    }

    // 2.1 call get pkc for dir
    Pkc *pkc = loader_get_pkc_for_dir(b, dir);

    // 2.2 search for correct namespace in config
    Allocator *alc = b->alc;
    char *name = NULL;

    if (pkc->config) {
        cJSON *json = pkc->config->json;
        cJSON *namespaces = cJSON_GetObjectItemCaseSensitive(json, "namespaces");
        if (namespaces) {
            cJSON *ns = namespaces->child;
            while (ns) {
                char ndir[KI_PATH_MAX];
                strcpy(ndir, pkc->dir);
                strcat(ndir, PATH_SLASH);
                strcat(ndir, ns->valuestring);
                fix_slashes(ndir, true);

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

    // 2.3 if found: load all namespace files
    if (name) {
        // Name found
        nsc = nsc_init(alc, b, pkc, name);

        Array *files = get_subfiles(alc, dir, false, true);
        int len = files->length;
        int i = 0;
        while (i < len) {
            char *fn = array_get_index(files, i);
            i++;
            if (!ends_with(fn, ".ki")) {
                continue;
            }

            char *path = al(alc, KI_PATH_MAX);
            strcpy(path, dir);
            strcat(path, fn);
            fc_init(b, path, nsc, false);
        }
    }

    // 2.4 if not found: use pkc->main_nsc (or create it if doesnt exist)
    if (!name) {
        nsc = pkc->main_nsc;
        if (!nsc) {
            nsc = nsc_init(alc, b, pkc, "main");
            pkc->main_nsc = nsc;
        }
    }

    // 2.5 set b->namespaces_by_dir[dir] = nsc
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
    fix_slashes(ndir, true);

    char dir_abs[KI_PATH_MAX];
    get_fullpath(ndir, dir_abs);

    if (!dir_exists(dir_abs)) {
        sprintf(b->sbuf, "Namespace directory not found: '%s'", dir_abs);
        build_error(b, b->sbuf);
    }

    //
    nsc = loader_get_nsc_for_dir(b, dir_abs);
    map_set(pkc->namespaces, name, nsc);

    return nsc;
}
