
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
        if (strcmp(action, "add") == 0 && args->length >= 3) {
            char *pkgname = array_get_index(args, 1);
            char *version = array_get_index(args, 2);
            char *alias = array_get_index(args, 3);
            pkg_add(pkgname, version, alias);
            return;
        } else if (strcmp(action, "remove") == 0 && args->length >= 2) {
            char *pkgname = array_get_index(args, 1);
            pkg_remove(pkgname);
            return;
        } else if (strcmp(action, "install") == 0) {
            pkg_install();
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

    printf("Commands: add|remove|install|upgrade\n\n");

    printf("> ki pkg add github.com/{user}/{pkgname} {version|head|latest} [optional:{local-pkg-name}]\n");
    printf("## Add a new package to your ki.json config & install it\n\n");

    printf("> ki pkg remove {local-pkg-name}\n");
    printf("## Remove a package from your project by package name defined in your ki.json config\n\n");

    printf("> ki pkg install\n");
    printf("## Install all packages defined in your ki.json\n\n");

    printf("> ki pkg upgrade {local-pkg-name}\n");
    printf("> ki pkg upgrade all\n");
    printf("## Upgrade (or downgrade) the version of an installed package\n\n");
    printf("## When using 'all' it will update all packages to the 'latest' version\n\n");
}
