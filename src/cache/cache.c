
#include "../all.h"

void cmd_cache(Array *args, Map *options) {
    //
    if (map_get(options, "-h")) {
        cache_help();
        return;
    }

    // Check args
    if (args->length >= 1) {
        char *action = array_get_index(args, 0);
        if (strcmp(action, "clean") == 0) {
            cache_clean();
            return;
        }
    }

    // Default
    cache_help();
}

void cache_help() {
    //
    printf("Usage: ki cache {command}\n\n");

    printf("Commands: clean\n\n");

    printf("> ki cache clean\n");
    printf("## Clean entire cache directory\n\n");
}
