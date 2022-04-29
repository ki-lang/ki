
#include "all.h"

Map *get_options_and_args(int argc, char *argv[], Map *option_has_value);

int main(int argc, char *argv[]) {
    //
    if (argc < 2) {
        main_print_help();
        return 1;
    }

    char *cmd = argv[1];

    Map *option_has_value = map_make();
    Map *params = get_options_and_args(argc, argv, option_has_value);
    Array *args = map_get(params, "args");
    Map *options = map_get(params, "options");

    if (strcmp(cmd, "build") == 0) {
        // Build
        cmd_build(args, options);
    } else if (strcmp(cmd, "pkg") == 0) {
        // Packages
        cmd_pkg(args, options);
    } else if (strcmp(cmd, "fmt") == 0) {
        // fmt
        // cmd_fmt(args, options);
    } else if (strcmp(cmd, "cache") == 0) {
        // Cache
        cmd_cache(args, options);
    } else if (strcmp(cmd, "version") == 0) {
        printf("ki version 0.0.1");
        // Todo: print os-target and cpu-arch
        printf("\n");
    } else {
        // Invalid command
        main_print_help();
        return 1;
    }

    return 0;
}

Map *get_options_and_args(int argc, char *argv[], Map *option_has_value) {
    Map *result = map_make();
    Array *args = array_make(2);
    Map *options = map_make();
    map_set(result, "args", args);
    map_set(result, "options", options);

    bool is_option;
    char *option_name;

    for (int i = 2; i < argc; i++) {
        is_option = false;
        if (argv[i][0] == '-') {
            is_option = true;
            option_name = argv[i];
            bool has_value = map_get(option_has_value, option_name);
            if (has_value) {
                i++;
                if (i == argc) {
                    break;
                }
            } else {
                map_set(options, option_name, "_");
                continue;
            }
        }
        if (is_option) {
            map_set(options, option_name, argv[i]);
        } else {
            array_push(args, argv[i]);
        }
    }

    return result;
}

void main_print_help() {
    printf("Usage: ki [command] -h\n\n");

    printf("Commands: build|pkg|fmt|cache|version\n\n");

    printf("> build    :  compile .ki code to executable\n");
    printf("> pkg      :  install/remove packages from github or other sources\n");
    printf("> fmt      :  format .ki code: indenting, newlines, etc...\n");
    printf("> cache    :  cache related operations. e.g. clean cache\n");
    printf("> version  :  prints the ki compiler version\n");

    printf("\n");
}