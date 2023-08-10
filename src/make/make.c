
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

    Array *args_a = array_make(alc, 4);
    Array *args_b = array_make(alc, 10);

    Array *add_to = args_a;
    for (int i = 2; i < args->length; i++) {
        char *arg = array_get_index(args, i);
        if (add_to == args_a && strcmp(arg, "--") == 0) {
            add_to = args_b;
            continue;
        }
        array_push(add_to, arg);
    }

    if (array_contains(args_a, "-h", arr_find_str) || array_contains(args_a, "--help", arr_find_str)) {
        cmd_make_help();
    }

    char *name = "default";
    char *dir = ".";

    if (args_a->length > 2) {
        name = array_get_index(args_a, 2);
    }
    if (args_a->length > 3) {
        dir = array_get_index(args_a, 3);
    }

    Str *buf = str_make(alc, 1000);
    Config *cfg = cfg_load(alc, buf, dir);
    if (!cfg) {
        printf("ki.json config not found in dir: '%s'\n", dir);
        exit(1);
    }
    cJSON *cmds = cJSON_GetObjectItemCaseSensitive(cfg->json, "make");
    if (!cmds) {
        printf("No make commands found\n");
        exit(1);
    }
    cJSON *cmd = cJSON_GetObjectItemCaseSensitive(cmds, name);
    if (!cmd || !cmd->valuestring) {
        printf("No make command found named: '%s'\n", name);
        exit(1);
    }

    Str *fcmd = str_make(alc, 101);
    str_append_chars(fcmd, cmd->valuestring);
    for (int i = 0; i < args_b->length; i++) {
        char *arg = array_get_index(args_b, i);
        str_append_chars(fcmd, " ");
        str_append_chars(fcmd, arg);
    }

    char *rcmd = str_to_chars(alc, fcmd);
    printf("Run: '%s'\n", rcmd);
    int code = system(rcmd);
    code = code == 0 ? 0 : 1;
    exit(code);
}

void cmd_make_help() {
    //
    printf("> ki make [optional:{command-name}] [optional:{config-dir-path}] [-- {append-to-command}]\n");
    printf("\n");

    printf("# If no command name is provided, then make will\n  run the command named \"default\"\n");
    printf("\n");

    printf("# If no directory path is given, it will look in your\n  current working directory for the ki.json config\n");
    printf("\n");

    printf("# Anything after '--' will be appended to the command that's being executed");
    printf("\n");

    exit(1);
}