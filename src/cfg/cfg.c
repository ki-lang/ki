
#include "../all.h"

void cmd_cfg(Array *args, Map *options) {
    //
    if (map_get(options, "-h")) {
        cfg_help();
        return;
    }

    if (!check_installed_git()) {
        die("Please install 'git' in order to install/upgrade ki packages");
    }

    // Check args
    if (args->length >= 1) {
        char *action = array_get_index(args, 0);
        if (strcmp(action, "init") == 0) {
            cfg_init();
            return;
        }
    }

    // Default
    cfg_help();
}

void cfg_help() {
    //
    printf("Usage: ki cfg {command}\n\n");

    printf("Commands: init\n\n");

    printf("> ki cfg init\n");
    printf("## Creates an empty ki.json config file in your current working directory\n\n");
}