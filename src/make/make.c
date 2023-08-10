
#include "../all.h"

void cmd_make_help();

void cmd_make(int argc, char *argv[]) {
    //
    //
    Allocator *alc = alc_make();

    Array *args = array_make(alc, argc);
    Map *options = map_make(alc);
    Array *has_value = array_make(alc, 8);

    parse_argv(argv, argc, has_value, args, options);

    if (array_contains(args, "-h", arr_find_str) || array_contains(args, "--help", arr_find_str)) {
        cmd_make_help();
    }

    printf("make\n");
}

void cmd_make_help() {
    //
    printf("> ki make [optional:{script-name}] [optional:{config-dir-path}]\n");
    printf("\n");

    printf("# If no script name is provided, the make command\n  will run the script named \"default\"\n");
    printf("\n");

    printf("# If no directory path is given, it will look in your\n  current working directory for the ki.json config\n");
    printf("\n");

    exit(1);
}