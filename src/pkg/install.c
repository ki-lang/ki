
#include "../all.h"
#include "../libs/httpclient.h"

void pkg_install() {
    //
    char cwd[KI_PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    Config *cfg = cfg_get(cwd);
    if (cfg == NULL) {
        die_token("No ki.json config not found in '%s'", cwd);
    }
    cJSON *pkgs = cJSON_GetObjectItemCaseSensitive(cfg->json, "packages");
    if (pkgs == NULL) {
        return;
    }
    cJSON *pkg = pkgs->child;
    while (pkg) {

        cJSON *_name = cJSON_GetObjectItemCaseSensitive(pkg, "name");

        if (_name == NULL) {
            die("There is a package in your ki.json config without a 'name' field");
        }

        cJSON *_version = cJSON_GetObjectItemCaseSensitive(pkg, "version");

        char clone_url[1024];
        char *name = _name->valuestring;
        char *version = NULL;

        if (_version)
            version = _version->valuestring;

        GithubPkg *ghub = github_parse_url(name);

        char *hash = NULL;
        if (ghub) {
            sprintf(clone_url, "git@github.com:%s/%s", ghub->username, ghub->pkgname);
            hash = github_find_version_hash(ghub, version);
        } else {
            die_token("Invalid pacakge name: '%s'", name);
        }

        pkg_install_package(cfg->dir, name, version, clone_url, hash);

        pkg = pkg->next;
    }
}

void pkg_install_package(char *dir, char *name, char *version, char *clone_url, char *hash) {
    // Download files
    char *dirname = strdup(name);
    // replace slashes
    int i = 0;
    while (true) {
        char ch = dirname[i];
        if (ch == 0)
            break;
        if (ch == '/')
            dirname[i] = '.';
        i++;
    }

    char pkgpath[KI_PATH_MAX];
    char versionpath[KI_PATH_MAX];
    strcpy(pkgpath, dir);
    strcat(pkgpath, "/packages");

    if (!file_exists(pkgpath)) {
        makedir(pkgpath, 0755);
    }

    strcat(pkgpath, "/");
    strcat(pkgpath, dirname);

    if (!file_exists(pkgpath)) {
        makedir(pkgpath, 0755);
    }

    char cmd[KI_PATH_MAX];
    char cmdout[1000];

    strcpy(versionpath, pkgpath);
    strcat(versionpath, "/");
    strcat(versionpath, version);

    if (!file_exists(versionpath)) {
        strcpy(cmd, "cd ");
        strcat(cmd, pkgpath);
        strcat(cmd, " && git clone ");
        strcat(cmd, clone_url);
        strcat(cmd, " ");
        strcat(cmd, version);
        strcat(cmd, " 2>&1");

        exec_simple(cmd, cmdout);
        // makedir(pkgpath, 0755);
    }

    strcpy(cmd, "cd ");
    strcat(cmd, versionpath);
    strcat(cmd, " && git checkout ");
    strcat(cmd, hash);
    strcat(cmd, " 2>&1");

    exec_simple(cmd, cmdout);

    //
    printf("[+] Installed '%s' (%s)\n", name, hash);
}
