
#include "../all.h"

void cmd_fmt_help();

void cmd_fmt(int argc, char *argv[]) {

    Allocator *alc = alc_make();
    Str *str_buf = str_make(alc, 5000);
    char *char_buf = al(alc, 5000);

    Array *args = array_make(alc, argc);
    Map *options = map_make(alc);
    Array *has_value = array_make(alc, 8);

    parse_argv(argv, argc, has_value, args, options);

    if (array_contains(args, "-h", arr_find_str) || array_contains(args, "--help", arr_find_str)) {
        cmd_fmt_help();
    }

    if (args->length < 3) {
        cmd_fmt_help();
    }

    Array *files = array_make(alc, argc);
    int argc_ = args->length;
    for (int i = 2; i < argc_; i++) {
        char *arg = array_get_index(args, i);
        if (arg[0] == '-') {
            continue;
        }

        char *full = al(alc, KI_PATH_MAX);
        bool success = get_fullpath(arg, full);

        if (!success || !file_exists(full)) {
            sprintf(char_buf, "fmt: file not found: '%s'", arg);
            die(char_buf);
        }

        if (!ends_with(arg, ".ki")) {
            sprintf(char_buf, "fmt: filename must end with .ki : '%s'", arg);
            die(char_buf);
        }

        array_push(files, full);
    }

    if (files->length == 0) {
        sprintf(char_buf, "Nothing to format, add some files to your fmt command");
        die(char_buf);
    }

    int filec = files->length;
    for (int i = 0; i < filec; i++) {
        char *path = array_get_index(files, i);
        fmt_format(alc, path);
    }
}

void fmt_format(Allocator *alc, char *path) {
    //
}

void cmd_fmt_help() {
    //
    printf("> ki fmt {ki file paths}\n");
    printf("\n");

    exit(1);
}
