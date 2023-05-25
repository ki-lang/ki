
#include "../all.h"

bool pkg_is_url(char *name) {
    //
    return strchr(name, '.') != NULL;
}

void pkg_add(PkgCmd *pc, char *name, char *version, char *alias) {
    //
    char *hash = NULL;
    Allocator *alc = pc->alc;
    char *cbuf = pc->char_buf;

    if (!alias) {
        alias = get_path_basename(alc, name);
        for (int i = 0; alias[i]; i++) {
            alias[i] = tolower(alias[i]);
        }
    }

    // char zipurl[1024];
    char cloneurl[1024];

    if (pkg_is_url(name)) {
        if (pkg_is_github_url(name)) {

            GithubPkg *ghub = github_parse_url(alc, name);
            if (!ghub) {
                sprintf(cbuf, "Invalid github URL: '%s'", name);
                die(cbuf);
            }

            if (!alias) {
                alias = strdup(ghub->pkgname);
                int len = strlen(alias);
                int o = 0;
                char lch = 0;
                for (int i = 0; i < len; i++) {
                    char ch = alias[i];
                    if (!is_valid_varname_char(ch)) {
                        ch = '_';
                        if (lch == '_') {
                            o--;
                        }
                    }
                    alias[o] = ch;
                    lch = ch;
                    o++;
                }
                alias[o] = '\0';
            }

            hash = github_find_version_hash(ghub, version);

            sprintf(cloneurl, "https://github.com/%s/%s.git", ghub->username, ghub->pkgname);
            // sprintf(zipurl, "https://%s/archive/%s.zip", name, hash);

        } else {
            sprintf(cbuf, "Invalid package repository URL: '%s'", name);
            die(cbuf);
        }
    } else {
        die("We currently only support packages via github urls\n");
    }

    if (!hash) {
        die("No version hash found");
    }

    // Add to ki.json
    char cwd[KI_PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    Config *cfg = cfg_load(alc, pc->str_buf, cwd);
    if (cfg == NULL) {
        sprintf(cbuf, "ki.json config not found in '%s'\n", cwd);
        die(cbuf);
        // die_token("ki.json config not found in '%s'\nuse: 'ki cfg init' to create a new config file", cwd);
    }
    if (cfg_has_package(cfg, alias)) {
        sprintf(cbuf, "There is already a package using the name: '%s', consider using an alias", alias);
        die(cbuf);
    }
    //
    cJSON *pkgs = cJSON_GetObjectItemCaseSensitive(cfg->json, "packages");
    if (pkgs == NULL) {
        pkgs = cJSON_CreateObject();
        cJSON_AddItemToObject(cfg->json, "packages", pkgs);
    }
    //
    cJSON *pkgob = cJSON_CreateObject();
    cJSON *cj_name = cJSON_CreateString(name);
    cJSON *cj_version = cJSON_CreateString(version);
    cJSON_AddItemToObject(pkgob, "name", cj_name);
    cJSON_AddItemToObject(pkgob, "version", cj_version);

    // Save config
    cJSON_AddItemToObject(pkgs, alias, pkgob);
    cfg_save(cfg);

    printf("[+] Package added '%s'\n", name);

    pkg_install_package(cfg->dir, name, version, cloneurl, hash);
}
