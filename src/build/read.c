
#include "../all.h"

int tok(Fc *fc, char *token, bool sameline, bool allow_space) {
    //
    int i = fc->i;
    char *content = fc->content;
    char ch = content[i];

    if (ch == '\0') {
        token[0] = '\0';
        return i;
    }

    if (!allow_space && is_whitespace(ch)) {
        token[0] = '\0';
        return i;
    }

    // Skip whitespace
    while (true) {
        while (is_whitespace(ch)) {
            //
            if (is_newline(ch)) {
                g_LOC++;
                if (sameline) {
                    token[0] = '\0';
                    return i;
                }
            }
            //
            i++;
            ch = content[i];
            if (ch == '\0') {
                token[0] = '\0';
                return i;
            }
        }
        // Skip comment
        if (!sameline && ch == '/' && content[i + 1] == '/') {
            i += 2;
            ch = content[i];
            while (!is_newline(ch)) {
                if (ch == '\0') {
                    token[0] = '\0';
                    return i;
                }
                i++;
                ch = content[i];
            }
        } else {
            break;
        }
    }

    int pos = 1;
    token[0] = ch;
    i++;

    if (is_number(ch)) {
        // Read number
        ch = content[i];
        while (is_number(ch)) {
            token[pos] = ch;
            i++;
            pos++;
            ch = content[i];
        }
    } else if (is_valid_varname_char(ch)) {
        // Read var name
        ch = content[i];
        while (is_valid_varname_char(ch)) {
            token[pos] = ch;
            i++;
            pos++;
            ch = content[i];
        }
    } else {
        // Special character
        char nch = content[i];
        if ((nch == '=' && (ch == '+' || ch == '-' || ch == '*' || ch == '/'))) {
            i++;
            token[pos] = nch;
            pos++;
        } else if ((ch == '&' && nch == '&') || (ch == '|' && nch == '|')) {
            i++;
            token[pos] = nch;
            pos++;
        } else if ((ch == '=' && nch == '=') || (ch == '!' && nch == '=') || (ch == '<' && nch == '=') || (ch == '>' && nch == '=')) {
            i++;
            token[pos] = nch;
            pos++;
        } else if ((ch == '<' && nch == '<') || (ch == '>' && nch == '>')) {
            i++;
            token[pos] = nch;
            pos++;
        } else if ((ch == '+' && nch == '+') || (ch == '-' && nch == '-')) {
            i++;
            token[pos] = nch;
            pos++;
        } else if ((ch == '-' && nch == '>') || (ch == '?' && nch == '?')) {
            i++;
            token[pos] = nch;
            pos++;
        } else if ((ch == '!' && nch == '!')) {
            i++;
            token[pos] = nch;
            pos++;
        }
    }

    token[pos] = '\0';

    // printf("tok: '%s'\n", token);

    return i;
}

int tok_expect(Fc *fc, char *expect, bool sameline, bool allow_space) {
    char token[KI_TOKEN_MAX];
    int i = tok(fc, token, sameline, allow_space);
    if (strcmp(token, expect) != 0) {
        sprintf(fc->sprintf, "Expected: '%s', but found: '%s'", expect, token);
        fc_error(fc, fc->sprintf, NULL);
    }
    return i;
}

char get_char(Fc *fc, int index) {
    //
    return fc->content[fc->i + index];
}
