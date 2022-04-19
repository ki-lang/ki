
#include "../all.h"

void fc_name_taken(FileCompiler *fc, Map *identifiers, char *name) {
    if (!is_valid_varname(name)) {
        fc_error(fc, "Invalid name, allowed characters [ a-z A-Z 0-9 _ ]: '%s'", name);
    }
    void *x = map_get(identifiers, name);
    if (x != NULL) {
        fc_error(fc, "Name already taken: '%s'", name);
    }
}

void fc_warn(FileCompiler *fc, char *msg, char *token) {
    char *fullmsg = malloc(5000);
    if (token == NULL) {
        strcpy(fullmsg, msg);
    } else {
        sprintf(fullmsg, msg, token);
    }
    printf("Warning: %s\n", fullmsg);
    //
    free(fullmsg);
}

void fc_error(FileCompiler *fc, char *msg, char *token) {
    int row = 1;
    int col = 1;
    int i = fc->i;
    while (i > 0) {
        i--;
        char ch = fc->content[i];
        if (is_newline(ch)) {
            row++;
        } else if (row == 1) {
            col++;
        }
    }

    printf("\n");
    printf("Error at line %d (col: %d) in file: %s\n", row, col, fc->ki_filepath);
    printf("Error: ");
    if (token == NULL) {
        printf("%s", msg);
    } else {
        printf(msg, token);
        fc->i -= strlen(token) + 1;
    }
    printf("\n");
    //   printf("Stage: %s\n", compile_stage);

    char *buf = malloc(2000);

    i = fc->i;
    int x = 0;
    while (i > 0) {
        i--;
        char ch = fc->content[i];
        if (ch == '\t') {
            ch = ' ';
        }
        if (is_newline(ch)) {
            break;
        }
        buf[x] = ch;
        x++;
    }
    buf[x] = '\0';

    char *code = malloc(2000);
    int y = 0;
    while (x > 0) {
        x--;
        code[y] = buf[x];
        y++;
    }
    i = fc->i;
    while (i < fc->content_len) {
        char ch = fc->content[i];
        i++;
        if (is_newline(ch)) {
            break;
        }
        if (is_whitespace(ch)) {
            ch = ' ';
        }
        code[y] = ch;
        y++;
    }
    code[y] = '\0';

    // printf("################################\n");
    for (i = 1; i < 20; i++) {
        printf("#");
    }
    printf("\n");
    printf("%s\n", code);
    for (i = 1; i < col - 2; i++) {
        printf(i < 20 ? "#" : " ");
    }
    printf(" ^ ");
    i += 3;
    for (i = i; i < 20; i++) {
        printf("#");
    }
    printf("\n");
    // printf("################################\n");

    // raise(SIGSEGV);  // Useful for debugging
    exit(1);
}
