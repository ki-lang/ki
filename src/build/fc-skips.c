
#include "../all.h"

void fc_skip_string(FileCompiler *fc) {
    char ch = '\0';
    char pch = '\0';
    while (fc->i < fc->content_len) {
        pch = ch;
        ch = fc->content[fc->i];
        fc->i++;
        if (ch == '"' && pch != '\\') {
            break;
        }
    }
}

void fc_skip_body(FileCompiler *fc, char *start, char *end, char *alt_end, bool sameline) {
    char *token = malloc(256);
    //
    int scope_depth = 1;
    while (fc->i < fc->content_len) {
        fc_next_token(fc, token, false, sameline && scope_depth == 1, true);
        if (strcmp(token, "\"") == 0) {
            fc_skip_string(fc);
            continue;
        }

        if (strcmp(token, "func") == 0) {
            fc_next_token(fc, token, false, false, true);
            // printf("Func:%s\n", token);
        }

        if (strcmp(token, start) == 0) {
            scope_depth++;
            // printf("U:%d\n", scope_depth);
            continue;
        }
        if (strcmp(token, end) == 0) {
            scope_depth--;
            // printf("D:%d\n", scope_depth);
            if (scope_depth == 0) {
                break;
            }
            continue;
        }

        if (strcmp(token, "/") == 0) {
            char nch = fc_get_char(fc, 0);
            if (nch == '/') {
                fc->i++;
                fc_skip_comment(fc);
                continue;
            }
        }

        if (alt_end != NULL && strcmp(token, alt_end) == 0) {
            if (scope_depth == 1) {
                scope_depth--;
                break;
            }
            continue;
        }

        if (allow_new_namespaces && is_valid_varname(token)) {
            fc->i -= strlen(token);
            Identifier *id = fc_read_identifier(fc, false, true, false);
            if (id == NULL) {
                continue;
            }
            if (!uses_async && id->package == NULL && id->namespace == NULL) {
                if (strcmp(id->name, "async") == 0) {
                    PkgCompiler *pkc = pkc_get_by_name("ki");
                    pkc_create_namespace(pkc, "async");
                    uses_async = true;
                }
                continue;
            }
        }
    }

    if (scope_depth != 0) {
        fc_error(fc, "Missing a '}' somewhere", NULL);
    }

    free(token);
}

void fc_skip_until_char(FileCompiler *fc, char ch) {
    while (fc->i < fc->content_len) {
        char nch = fc->content[fc->i];
        if (nch == ch) {
            break;
        }
        fc->i++;
    }
}

void fc_skip_assign_value(FileCompiler *fc) {
    while (fc->i < fc->content_len) {
        char nch = fc->content[fc->i];
        if (nch == ';') {
            break;
        }
        fc->i++;
        if (nch == '\"') {
            fc_skip_string(fc);
            continue;
        }
        if (nch == '(') {
            fc_skip_body(fc, "(", ")", NULL, false);
            continue;
        }
        if (nch == '[') {
            fc_skip_body(fc, "[", "]", NULL, false);
            continue;
        }
        if (nch == '{') {
            fc_skip_body(fc, "{", "}", NULL, false);
            continue;
        }
    }
}

void fc_skip_comment(FileCompiler *fc) {
    while (fc->i < fc->content_len) {
        char nch = fc->content[fc->i];
        if (is_newline(nch)) {
            break;
        }
        fc->i++;
    }
}

void fc_skip_type(FileCompiler *fc) {
    //
    char *token = malloc(KI_TOKEN_MAX);
    fc_next_token(fc, token, false, false, true);
    if (strcmp(token, "!") == 0) {
        fc_next_token(fc, token, false, false, true);
    }
    if (strcmp(token, "?") == 0) {
        fc_next_token(fc, token, false, false, true);
        if (strcmp(token, "!") == 0) {
            fc_error(fc, "put '!' before the '?'", NULL);
        }
    }

    fc->i -= strlen(token);

    Identifier *id = fc_read_identifier(fc, false, true, false);

    char ch = fc_get_char(fc, 0);
    if (ch == '<') {
        fc->i++;
        fc_skip_type(fc);
        fc_next_token(fc, token, true, true, true);
        while (strcmp(token, ">") != 0) {
            fc_expect_token(fc, ",", false, true, true);
            fc_skip_type(fc);
            fc_next_token(fc, token, true, true, true);
        }
        fc_next_token(fc, token, false, true, true);
    }

    //
    free(token);
    free_id(id);
}

void fc_skip_macro(FileCompiler *fc) {
    //
    char *token = malloc(KI_TOKEN_MAX);
    int depth = 1;
    while (true) {
        if (fc->i >= fc->content_len) {
            fc_error(fc, "Unable to find end of macro if statement", NULL);
        }

        fc_skip_body(fc, "--IGNORE--", "#", NULL, false);
        fc_next_token(fc, token, false, true, false);
        if (strcmp(token, "if") == 0) {
            depth++;
        } else if (strcmp(token, "elif") == 0 || strcmp(token, "else") == 0) {
            if (depth == 1) {
                fc->i -= 5;
                break;
            }
        } else if (strcmp(token, "end") == 0) {
            depth--;
            if (depth == 0) {
                fc->i -= 4;
                break;
            }
        }
    }
}