
#include "all.h"

void help();

int main(int argc, char *argv[]) {
    //
    if (argc < 2) {
        help();
    }

    char *cmd = argv[1];

    if (strcmp(cmd, "build") == 0) {
        cmd_build(argc, argv);
    } else {
        help();
    }
}

void help() {
    //
    printf("\n");
    printf("# ki build -h\n");
    printf("\n");
    exit(1);
}
