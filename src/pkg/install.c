
#include "../all.h"

void pkg_install(PkgCmd *pc) {
    //
    Allocator *alc = pc->alc;
    char *cbuf = pc->char_buf;

    char cwd[KI_PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    Config *cfg = cfg_load(alc, pc->str_buf, cwd);
    if (cfg == NULL) {
        sprintf(cbuf, "No ki.json config not found in '%s'", cwd);
        die(cbuf);
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

        GithubPkg *ghub = github_parse_url(alc, name);

        char *hash = NULL;
        if (ghub) {
            sprintf(clone_url, "git@github.com:%s/%s", ghub->username, ghub->pkgname);
            hash = github_find_version_hash(ghub, version);
        } else {
            sprintf(cbuf, "Invalid pacakge name: '%s'", name);
            die(cbuf);
        }

        pkg_install_package(pc, cfg->dir, name, version, clone_url, hash);

        pkg = pkg->next;
    }
}

void pkg_get_dir(char *packages_dir, char *name, char *buf) {
    //
    char dirname[256];
    strcpy(dirname, name);
    // replace slashes
    int i = 0;
    while (true) {
        char ch = dirname[i];
        if (ch == 0)
            break;
        if (ch == '/')
            dirname[i] = '_';
        i++;
    }
    str_replace_simple(dirname, "github.com", "github");
    strcpy(buf, packages_dir);
    strcat(buf, "/");
    strcat(buf, dirname);
}

bool pkg_install_package(PkgCmd *pc, char *dir, char *name, char *version, char *clone_url, char *hash) {
    // Download files
    char pkgdir[KI_PATH_MAX];
    char pkgpath[KI_PATH_MAX];
    char versionpath[KI_PATH_MAX];
    strcpy(pkgdir, dir);
    strcat(pkgdir, "/packages");

    if (!file_exists(pkgdir)) {
        makedir(pkgdir, 0755);
    }

    pkg_get_dir(pkgdir, name, pkgpath);

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
        strcat(cmd, " --quiet");

        system_silent(cmd);
        // makedir(pkgpath, 0755);
    }

    strcpy(cmd, "cd ");
    strcat(cmd, versionpath);
    strcat(cmd, " && git fetch --quiet && git checkout ");
    strcat(cmd, hash);
    strcat(cmd, " --quiet");

    system_silent(cmd);

    // Validate package
    char *cbuf = pc->char_buf;
    Config *cfg = cfg_load(pc->alc, pc->str_buf, versionpath);
    if (cfg == NULL) {
        printf("Package has no 'ki.json' configuration file: '%s'\n", name);
        return false;
    }
    cJSON *json = cfg->json;
    bool fail = false;
    cJSON *jname = cJSON_GetObjectItemCaseSensitive(json, "name");
    if (jname == NULL) {
        printf("Missing 'name' property in: '%s'\n", cfg->path);
        fail = true;
    } else {
        char *_name = jname->valuestring;
        if (!is_valid_varname_all(_name)) {
            printf("Invalid package name syntax in: '%s'\nMame was '%s', allowed characters [a-zA-Z0-9_] and first character cannot be a number.\n", cfg->path, _name);
            fail = true;
        }
    }
    cJSON *jversion = cJSON_GetObjectItemCaseSensitive(json, "version");
    if (jversion == NULL) {
        printf("Missing 'version' property in: '%s'\n", cfg->path);
        fail = true;
    } else {
        char *_version = jversion->valuestring;
        PkgVersion *v = extract_version(_version);
        if (!v) {
            printf("Invalid package version syntax in: '%s'\nVersion was '%s', format must be: [0-9].[0-9].[0-9]\n", cfg->path, _version);
            fail = true;
        }
    }
    if (fail) {
        printf("Package '%s' was not correctly configured\n", name);
        return false;
    }

    printf("[+] Installed '%s' (%s)\n", name, hash);

    return true;
}
