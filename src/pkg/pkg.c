
#include "../all.h"

void cmd_pkg(Array *args, Map *options) {
    //
    if (map_get(options, "-h")) {
        cmd_pkg_help();
        return;
    }

    cmd_pkg_help();
}

void cmd_pkg_help() {
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
