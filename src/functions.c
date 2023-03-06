
#include "all.h"

void die(char *msg) {
    printf("%s\n", msg);
    exit(1);
}

void parse_argv(char *argv, int argc, Array *has_value, Array *args, Map *options) {
    //
    for (int i = 0; i < argc; i++) {
        char *arg = argv[i];
        int index = array_find(has_value, arg, "chars");
        if (index == -1) {
            array_push(args, arg);
            continue;
        }
        i++;
        if (i == argc) {
            break;
        }
        char *value = argv[i];
        map_set(options, arg, value);
    }
}
