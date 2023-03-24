
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
    } else if (strcmp(cmd, "run") == 0) {
        cmd_build(argc, argv);
    } else {
        help();
    }
}

void help() {
    //
    printf("-------------------------\n");
    printf(" ki lang compiler v0.1\n");
    printf("-------------------------\n\n");

    printf(" ki build -h       Build ki code to an executable\n");
    printf(" ki run -h         Build and run ki code\n");

    printf("\n");
    exit(1);
}
