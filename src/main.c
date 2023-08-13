
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
    } else if (strcmp(cmd, "pkg") == 0) {
        cmd_pkg(argc, argv);
    } else if (strcmp(cmd, "make") == 0) {
        cmd_make(argc, argv);
    } else if (strcmp(cmd, "ls") == 0) {
        cmd_lsp(argc, argv);
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
    printf(" ki pkg -h         Manage packages\n");
    printf(" ki make -h        Run pre-defined scripts\n");

    printf("\n");
    printf(" ki fmt -h         Format ki code\n");
    printf(" ki ls -h          Run language server\n");

    printf("\n");
    exit(1);
}
