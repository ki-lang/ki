
#include "../all.h"

void cmd_pkg_help();

void cmd_pkg(int argc, char *argv[]) {
    //
    Allocator *alc = alc_make();

    Array *args = array_make(alc, argc);
    Map *options = map_make(alc);
    Array *has_value = array_make(alc, 8);

    parse_argv(argv, argc, has_value, args, options);

    if (array_contains(args, "-h", arr_find_str) || array_contains(args, "--help", arr_find_str) || args->length < 3) {
        cmd_pkg_help();
    }

    char *action = array_get_index(args, 2);
    PkgCmd *pc = al(alc, sizeof(PkgCmd));
    pc->alc = alc;
    pc->sbuf = al(alc, 2000);

    if (strcmp(action, "add") == 0) {
        if (args->length < 4) {
            die("Missing package name/url");
        }
        if (args->length < 5) {
            die("Missing package version");
        }

        char *name = array_get_index(args, 3);
        char *version = array_get_index(args, 4);
        char *local_name = NULL;

        if (args->length > 5) {
            local_name = array_get_index(args, 5);
        }

    } else if (strcmp(action, "remove") == 0) {
    } else if (strcmp(action, "install") == 0) {
    } else {
        cmd_pkg_help();
    }
}

void cmd_pkg_help() {
    //
    printf(" _    _         _         \n");
    printf("| |  (_)       | |        \n");
    printf("| | ___   _ __ | | ____ _ \n");
    printf("| |/ / | | '_ \\| |/ / _` |\n");
    printf("|   <| | | |_) |   < (_| |\n");
    printf("|_|\\_\\_| | .__/|_|\\_\\__, |\n");
    printf("         | |         __/ |\n");
    printf("         |_|        |___/ \n");
    printf("\n");

    printf("# Add new packages to ki.json & install\n");
    printf("> ki pkg add github.com/{user}/{pkgname} {version|latest|commit} [optional:{local-pkg-name}]\n");
    printf("\n");

    printf("# Remove package from ki.json\n");
    printf("> ki pkg remove {local-pkg-name}\n");
    printf("\n");

    printf("# Install packages defined in ki.json\n");
    printf("> ki pkg install\n");
    printf("\n");

    exit(1);
}