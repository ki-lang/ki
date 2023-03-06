
#include "../all.h"

void cmd_build(int argc, char *argv[]) {
    //
    Array *args = array_make(argc);
    Map *options = map_make();
    Array *has_value = array_make(8);
    array_push(has_value, "-o");
    array_push(has_value, "--arch");
    array_push(has_value, "--os");

    parse_argv(argv, argc, has_value, args, options);

    // Check options

    //
}