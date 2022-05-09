
#include "../all.h"

void cmd_pkg(Array *args, Map *options) {
    //
    if (map_get(options, "-h")) {
        pkg_help();
        return;
    }

    if (!check_installed_git()) {
        die("Please install 'git' in order to install/upgrade ki packages");
    }

    // Check args
    if (args->length >= 1) {
        char *action = array_get_index(args, 0);
        if (strcmp(action, "install") == 0 && args->length >= 3) {
            char *pkgname = array_get_index(args, 1);
            char *version = array_get_index(args, 2);
            char *alias = array_get_index(args, 3);
            pkg_install(pkgname, version, alias);
            return;
        } else if (strcmp(action, "remove") == 0 && args->length >= 2) {
            char *pkgname = array_get_index(args, 1);
            pkg_remove(pkgname);
            return;
        } else if (strcmp(action, "upgrade") == 0 && args->length >= 3) {
            char *pkgname = array_get_index(args, 1);
            char *version = array_get_index(args, 2);
            pkg_upgrade(pkgname, version);
            return;
        }
    }

    // Default
    pkg_help();
}

void pkg_help() {
    //
    printf("Usage: ki pkg {command}\n\n");

    printf("Commands: install|upgrade|remove\n\n");

    printf("> ki pkg install github.com/{user}/{pkgname} {version|head|latest} [optional:{local-pkg-name}]\n");
    printf("## Install new package via package name or github URL\n\n");

    printf("> ki pkg remove {local-pkg-name}\n");
    printf("## Remove a package from your project by package name defined in your ki.json file\n\n");

    printf("> ki pkg upgrade {local-pkg-name} {version|head|latest}\n");
    printf("> ki pkg upgrade all\n");
    printf("## Upgrade (or downgrade) the version of an installed package\n\n");
    printf("## When using 'all' it will update all packages to the 'latest' version\n\n");
}
