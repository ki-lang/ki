
#include "../all.h"

void cmd_pkg_help();

void cmd_pkg(int argc, char *argv[]) {
    //
    Allocator *alc = alc_make();

    Array *args = array_make(alc, argc);
    Map *options = map_make(alc);
    Array *has_value = array_make(alc, 8);

    char *err = NULL;
    parse_argv(argv, argc, has_value, args, options, err);
    if (err) {
        die(err);
    }

    if (array_contains(args, "-h", arr_find_str) || array_contains(args, "--help", arr_find_str) || args->length < 3) {
        cmd_pkg_help();
    }

    char *action = array_get_index(args, 2);
    PkgCmd *pc = al(alc, sizeof(PkgCmd));
    pc->alc = alc;
    pc->char_buf = al(alc, 2000);
    pc->str_buf = str_make(alc, 2000);

    if (system_silent("git --version") != 0) {
        sprintf(pc->char_buf, "Is 'git' installed correctly? Could not execute 'git --version'");
        die(pc->char_buf);
    }

    if (strcmp(action, "add") == 0) {
        if (args->length < 4) {
            die("Missing package name/url");
        }
        if (args->length < 5) {
            die("Missing package version");
        }

        char *name = array_get_index(args, 3);
        char *version = array_get_index(args, 4);
        char *alias = NULL;

        if (args->length > 5) {
            alias = array_get_index(args, 5);
        }

        pkg_add(pc, name, version, alias);

    } else if (strcmp(action, "remove") == 0) {
        if (args->length < 4) {
            die("Missing package name");
        }
        char *name = array_get_index(args, 3);
        pkg_remove(pc, name);
    } else if (strcmp(action, "install") == 0) {
        pkg_install(pc);
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