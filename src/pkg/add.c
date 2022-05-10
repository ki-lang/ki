
#include "../all.h"

void pkg_add(char *name, char *version, char *alias) {
    //
    char *hash = NULL;
    // char zipurl[1024];
    char cloneurl[1024];

    if (pkg_is_url(name)) {
        if (pkg_is_github_url(name)) {

            GithubPkg *ghub = github_parse_url(name);
            if (!ghub) {
                die_token("Invalid github URL: '%s'", name);
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

            sprintf(cloneurl, "git@github.com:%s/%s", ghub->username, ghub->pkgname);
            // sprintf(zipurl, "https://%s/archive/%s.zip", name, hash);

        } else {
            die_token("Invalid package repository URL: '%s'", name);
        }
    } else {
        die("We currently only support packages via github urls\n");
    }

    if (!hash) {
        die("No version hash found");
    }

    // Add to ki.json
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    Config *cfg = cfg_get(cwd);
    if (cfg == NULL) {
        die_token("ki.json config not found in '%s'\nuse: 'ki cfg init' to create a new config file", cwd);
    }
    if (cfg_has_package(cfg, alias)) {
        die_token("There is already a package using the name: '%s', consider using an alias", alias);
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

    pkg_install_package(name, version, cloneurl, hash);
}