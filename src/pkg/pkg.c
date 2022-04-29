
#include "../all.h"

void cmd_pkg(Array *args, Map *options) {
    //
    if (map_get(options, "-h")) {
        pkg_help();
        return;
    }

    // Check args
    if (args->length >= 1) {
        char *action = array_get_index(args, 0);
        if (strcmp(action, "install") == 0 && args->length >= 3) {
            char *pkgname = array_get_index(args, 1);
            char *version = array_get_index(args, 2);
            pkg_install(pkgname, version);
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

    printf("> ki pkg install {pkg-name|repo-url} {version|head|latest}\n");
    printf("## Install new package via package name or github URL\n\n");

    printf("> ki pkg remove {local-pkg-name}\n");
    printf("## Remove a package from your project by package name defined in your ki.json file\n\n");

    printf("> ki pkg upgrade {local-pkg-name} {version|head|latest}\n");
    printf("> ki pkg upgrade all\n");
    printf("## Upgrade (or downgrade) the version of an installed package\n\n");
}
