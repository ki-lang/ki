
#include "../all.h"

void pkg_remove(char *name) {
    //
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    Config *cfg = cfg_get(cwd);
    if (cfg == NULL) {
        die_token("No ki.json config not found in '%s'", cwd);
    }
    cJSON *pkgs = cJSON_GetObjectItemCaseSensitive(cfg->json, "packages");
    if (pkgs == NULL) {
        die("You have no 'packages' defined in your ki.json");
    }

    // New config
    cJSON *newcfg = cJSON_CreateObject();
    // Copy content except "packages"
    cJSON *item = cfg->json->child;
    while (item) {
        if (strcmp(item->string, "packages") != 0) {
            cJSON_AddItemToObject(newcfg, item->string, item);
        }
        item = cfg->json->next;
    }

    // New package list
    cJSON *newpkgs = cJSON_CreateObject();
    // Loop packages
    bool found = false;
    cJSON *pkg = pkgs->child;
    while (pkg) {

        if (strcmp(pkg->string, name) == 0) {
            // Found
            found = true;
        } else {
            // Add to result
            cJSON_AddItemToObject(newpkgs, pkg->string, pkg);
        }

        pkg = pkg->next;
    }

    if (!found) {
        die_token("Package '%s' not found in your ki.json config", name);
    }

    cJSON_AddItemToObject(newcfg, "packages", newpkgs);

    // Update result
    cfg->json = newcfg;
    cfg_save(cfg);
}