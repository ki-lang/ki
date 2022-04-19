
#include "all.h"

Map *get_options_and_args(int argc, char *argv[], Map *option_has_value);

int main(int argc, char *argv[]) {
    //
    if (argc < 2) {
        main_print_help();
        return 1;
    }

    char *cmd = argv[1];

    if (strcmp(cmd, "build") == 0) {
        // Build
        Map *option_has_value = map_make();
        map_set(option_has_value, "-h", false);
        Map *params = get_options_and_args(argc, argv, option_has_value);
        Array *args = map_get(params, "args");
        Map *options = map_get(params, "options");
        cmd_build(args, options);
    } else if (strcmp(cmd, "fmt") == 0) {
        // fmt
        // Map* option_has_value = map_make();
        // Map* params = get_options_and_args(argc, argv, option_has_value);
        // Array* args = map_get(params, "args");
        // Map* options = map_get(params, "options");
        // cmd_fmt(args, options);
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
    printf("Usage: ki [command] -h\n");
    printf("Commands: build\n");
}