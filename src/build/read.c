
#include "../all.h"

Chunk *chunk_init(Allocator *alc) {
    Chunk *ch = al(alc, sizeof(Chunk));
    ch->content = NULL;
    ch->i = 0;
    ch->line = 1;
}
Chunk *chunk_clone(Allocator *alc, Chunk *chunk) {
    //
    Chunk *ch = al(alc, sizeof(Chunk));
    *ch = *chunk;
    return ch;
}
void chunk_move(Chunk *chunk, int pos) {
    //
    chunk->i += pos;
}

void tok(Fc *fc, char *token, bool sameline, bool allow_space) {
    //
    Chunk *chunk = fc->chunk;
    *fc->chunk_prev = *chunk;

    int i = chunk->i;
    const char *content = chunk->content;
    char ch = content[i];

    if (ch == '\0') {
        token[0] = '\0';
        return;
    }

    if (!allow_space && is_whitespace(ch)) {
        token[0] = '\0';
        return;
    }

    // Skip whitespace
    while (true) {
        while (is_whitespace(ch)) {
            //
            if (is_newline(ch)) {
                chunk->line++;
                if (sameline) {
                    token[0] = '\0';
                    chunk->i = i;
                    return;
                }
            }
            //
            i++;
            ch = content[i];
            if (ch == '\0') {
                token[0] = '\0';
                chunk->i = i;
                return;
            }
        }
        // Skip comment
        if (!sameline && ch == '/' && content[i + 1] == '/') {
            i += 2;
            ch = content[i];
            while (!is_newline(ch)) {
                if (ch == '\0') {
                    token[0] = '\0';
                    chunk->i = i;
                    return;
                }
                i++;
                ch = content[i];
            }
            chunk->line++;
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

    chunk->i = i;
}

void rtok(Fc *fc) { *fc->chunk = *fc->chunk_prev; }

void tok_expect(Fc *fc, char *expect, bool sameline, bool allow_space) {
    char token[KI_TOKEN_MAX];
    tok(fc, token, sameline, allow_space);
    if (strcmp(token, expect) != 0) {
        sprintf(fc->sbuf, "Expected: '%s', but found: '%s'", expect, token);
        fc_error(fc);
    }
}

char get_char(Fc *fc, int index) {
    //
    Chunk *chunk = fc->chunk;
    return chunk->content[chunk->i + index];
}

Str *read_string(Fc *fc) {
    //
    Str *buf = fc->b->str_buf;
    Chunk *chunk = fc->chunk;
    str_clear(buf);

    return buf;
}
