#include "../all.h"

void skip_body(Fc *fc, char until_ch) {
    //
    Chunk* chunk = fc->chunk;
    chunk->i = chunk->scope_end_i;
}

void skip_string(Fc *fc, char end_char) {
    //
    Chunk *chunk = fc->chunk;
    char ch;
    int i = chunk->i;
    int col = chunk->col;
    const char *content = chunk->content;
    while (chunk->i < chunk->length) {
        //
        ch = chunk->content[i];
        i++;
        col++;

        if (is_newline(ch)) {
            chunk->line++;
            col = 1;
        }

        if (ch == '\\') {
            i++;
            col++;
            continue;
        }

        if (ch == end_char) {
            break;
        }
    }

    chunk->i = i;
    chunk->col = col;

    if (chunk->i == chunk->length) {
        sprintf(fc->sbuf, "Unexpected end of code, string not closed");
        fc_error(fc);
    }
}

void skip_until_char(Fc *fc, char *find) {
    //
    char* token = "";
    Chunk* chunk = fc->chunk;
    while(strcmp(token, find) != 0 || fc->chunk->token == tok_string) {
        if(fc->chunk->token == tok_eof) {
            sprintf(fc->sbuf, "End of file, missing '%s' token", find);
            fc_error(fc);
        }
        token = tok(fc, NULL, false, true);
    }
}

void skip_whitespace(Fc *fc) {
    //
    Chunk *chunk = fc->chunk;
    int x = chunk->i;
    tok_read(chunk, &x);
    if(chunk->token == tok_space) {
        chunk->i = x;
        tok_read(chunk, &x);
    }
    if(chunk->token == tok_newline) {
        chunk->i = x;
    }
}

void skip_macro_if(Fc *fc) {
    //
    Chunk *chunk = fc->chunk;
    int depth = 1;
    while (true) {
        char* token = tok(fc, NULL, false, true);
        if(chunk->token == tok_cc) {
            if(strcmp(token, "if") == 0) {
                depth++;
            } else if(strcmp(token, "elif") == 0 || strcmp(token, "else")) {
                if(depth == 1) {
                    depth--;
                    break;
                }
            } else if(strcmp(token, "end") == 0) {
                depth--;
                if(depth == 0)
                    break;
            }
        } else if(chunk->token == tok_eof) {
            break;
        }
    }

    if (depth != 0) {
        sprintf(fc->sbuf, "End of file, missing #end token");
        fc_error(fc);
    }

    rtok(fc);
    skip_whitespace(fc);
}

void skip_traits(Fc *fc) {
    //
    char *token = fc->token;
    while (true) {
        tok(fc, token, false, true);
        if (is_valid_varname_char(token[0])) {
            rtok(fc);
            read_id(fc, false, true, true);
            continue;
        }
        if (token[0] != ',') {
            rtok(fc);
            break;
        }
    }
}

void skip_value(Fc *fc) {

    char *token = fc->token;
    while (true) {

        tok(fc, token, false, true);

        if (strcmp(token, "\"") == 0) {
            skip_string(fc, '"');
            continue;
        }
        if (strcmp(token, "'") == 0) {
            skip_string(fc, '\'');
            continue;
        }
        if (strcmp(token, "(") == 0) {
            skip_until_char(fc, ")");
            continue;
        }
        if (strcmp(token, "{") == 0) {
            skip_until_char(fc, "}");
            continue;
        }
        if (strcmp(token, "[") == 0) {
            skip_until_char(fc, "]");
            continue;
        }
        if (is_valid_varname_char(token[0]))
            continue;
        if (is_number(token[0]))
            continue;

        if (strcmp(token, ":") == 0 || strcmp(token, ".") == 0 || strcmp(token, "<=") == 0 || strcmp(token, ">=") == 0 || strcmp(token, "==") == 0 || strcmp(token, "!=") == 0 || strcmp(token, "&&") == 0 || strcmp(token, "||") == 0 || strcmp(token, "+") == 0 || strcmp(token, "-") == 0 || strcmp(token, "/") == 0 || strcmp(token, "*") == 0 || strcmp(token, "%") == 0 || strcmp(token, "&") == 0 || strcmp(token, "|") == 0 || strcmp(token, "^") == 0 || strcmp(token, "++") == 0 || strcmp(token, "--") == 0 || strcmp(token, "->") == 0 || strcmp(token, "??") == 0 || strcmp(token, "?!") == 0 || strcmp(token, "!!") == 0 || strcmp(token, "?") == 0) {
            continue;
        }

        rtok(fc);
        break;
    }
}

void skip_type(Fc *fc) {
    //
    Chunk *chunk = fc->chunk;

    char * token = tok(fc, NULL, false, true);
    if(strcmp(token, "raw") == 0 || strcmp(token, "weak") == 0){
        token = tok(fc, NULL, true, true);
    }
    while (strcmp(token, "?") == 0 || strcmp(token, ".") == 0 || strcmp(token, "&") == 0 || strcmp(token, "+") == 0) {
        token = tok(fc, NULL, true, false);
    }
    if(chunk->token != tok_id && token[0] != ':') {
        sprintf(fc->sbuf, "Expected a type here");
        fc_error(fc);
    }
    token = tok(fc, NULL, true, false);
    if(token[0] == '[') {
        skip_body(fc, ']');
    } else {
        rtok(fc);
    }
}

void skip_macro_input(Fc *fc, char *end) {

    char *token = fc->token;
    while (true) {

        tok(fc, token, false, true);

        if (token[0] == '\0') {
            sprintf(fc->sbuf, "Your macro is missing a closing tag, unexpected end of file");
            fc_error(fc);
        }

        if (strcmp(token, "\"") == 0) {
            skip_string(fc, '"');
            continue;
        }
        if (strcmp(token, "'") == 0) {
            skip_string(fc, '\'');
            continue;
        }
        if (strcmp(token, "(") == 0) {
            skip_until_char(fc, ")");
            continue;
        }
        if (strcmp(token, "{") == 0) {
            skip_until_char(fc, "}");
            continue;
        }
        if (strcmp(token, "[") == 0) {
            skip_until_char(fc, "]");
            continue;
        }
        if (strcmp(token, end) == 0 || strcmp(token, ",") == 0) {
            rtok(fc);
            break;
        }
    }
}