
#include "../all.h"

Chunk *chunk_init(Allocator *alc, Fc *fc) {
    Chunk *ch = al(alc, sizeof(Chunk));
    ch->parent = NULL;
    ch->fc = fc;
    ch->content = NULL;
    ch->length = 0;
    ch->i = 0;
    ch->line = 1;
    ch->col = 1;

    return ch;
}
Chunk *chunk_clone(Allocator *alc, Chunk *chunk) {
    //
    Chunk *ch = al(alc, sizeof(Chunk));
    *ch = *chunk;
    return ch;
}
void chunk_move(Chunk *chunk, int pos) {
    //
    int i = chunk->i;
    while (pos > 0) {
        pos--;
        char ch = chunk->content[chunk->i];
        chunk->i++;
        chunk->col++;
        if (ch == '\n') {
            chunk->line++;
            chunk->col = 0;
        }
    }
    while (pos < 0) {
        pos++;
        char ch = chunk->content[chunk->i];
        chunk->i--;
        chunk->col--;
        if (ch == '\n') {
            chunk->line--;
            int x = chunk->i;
            int col = 0;
            while (x > 0 && chunk->content[x] != '\n') {
                x--;
                col++;
            }
            chunk->col = col;
        }
    }
}
void chunk_update_col(Chunk *chunk) {
    //
    int col = 1;
    int i = chunk->i;
    char *content = chunk->content;
    while (i > 0) {
        i--;
        col++;
        char ch = content[i];
        if (ch == '\n')
            break;
    }
    chunk->col = col;
}

void tok(Fc *fc, char *token, bool sameline, bool allow_space) {
    //
    Chunk *chunk = fc->chunk;
    *fc->chunk_prev = *chunk;

    int i = chunk->i;
    int col = chunk->col;
    const char *content = chunk->content;
    char ch = content[i];

    while (ch == '\0') {
        if (chunk->parent) {
            chunk = chunk->parent;
            fc->chunk = chunk;
            *fc->chunk_prev = *chunk;
            i = chunk->i;
            col = chunk->col;
            content = chunk->content;
            ch = content[i];
            continue;
        }
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
                col = 1;
                if (sameline) {
                    token[0] = '\0';
                    chunk->i = i;
                    chunk->col = col;
                    return;
                }
            }
            //
            i++;
            col++;
            ch = content[i];
            if (ch == '\0') {
                if (chunk->parent) {
                    chunk->i = i;
                    chunk->col = col;
                    chunk = chunk->parent;
                    fc->chunk = chunk;
                    *fc->chunk_prev = *chunk;
                    i = chunk->i;
                    col = chunk->col;
                    content = chunk->content;
                    ch = content[i];
                    if (ch != '\0') {
                        continue;
                    }
                }
                token[0] = '\0';
                chunk->i = i;
                chunk->col = col;
                return;
            }
        }
        // Skip comment
        if (!sameline && ch == '/' && content[i + 1] == '/') {
            i += 2;
            col += 2;
            ch = content[i];
            while (!is_newline(ch)) {
                if (ch == '\0') {
                    token[0] = '\0';
                    chunk->i = i;
                    chunk->col = col;
                    return;
                }
                i++;
                col++;
                ch = content[i];
            }
            i++;
            col++;
            ch = content[i];
            chunk->line++;
            chunk->col = 1;
        } else {
            break;
        }
    }

    int pos = 1;
    token[0] = ch;
    i++;
    col++;

    if (is_number(ch)) {
        // Read number
        ch = content[i];
        while (is_number(ch)) {
            token[pos] = ch;
            i++;
            col++;
            pos++;
            ch = content[i];
        }
    } else if (is_valid_varname_char(ch) || ch == '@') {
        // Read var name
        ch = content[i];
        while (is_valid_varname_char(ch)) {
            token[pos] = ch;
            i++;
            pos++;
            col++;
            ch = content[i];
        }
    } else {
        // Special character
        char nch = content[i];
        if ((nch == '=' && (ch == '+' || ch == '-' || ch == '*' || ch == '/'))) {
            i++;
            col++;
            token[pos] = nch;
            pos++;
        } else if ((ch == '&' && nch == '&') || (ch == '|' && nch == '|')) {
            i++;
            col++;
            token[pos] = nch;
            pos++;
        } else if ((ch == '=' && nch == '=') || (ch == '!' && nch == '=') || (ch == '<' && nch == '=') || (ch == '>' && nch == '=')) {
            i++;
            col++;
            token[pos] = nch;
            pos++;
        } else if ((ch == '<' && nch == '<') || (ch == '>' && nch == '>')) {
            i++;
            col++;
            token[pos] = nch;
            pos++;
        } else if ((ch == '+' && nch == '+') || (ch == '-' && nch == '-')) {
            i++;
            col++;
            token[pos] = nch;
            pos++;
        } else if ((ch == '-' && nch == '>')) {
            i++;
            col++;
            token[pos] = nch;
            pos++;
        } else if ((ch == '?' && nch == '?') || (ch == '?' && nch == '!')) {
            i++;
            col++;
            token[pos] = nch;
            pos++;
        } else if ((ch == '!' && nch == '!') || (ch == '!' && nch == '?')) {
            i++;
            col++;
            token[pos] = nch;
            pos++;
        } else if ((ch == '{' && nch == '{') || (ch == '}' && nch == '}')) {
            i++;
            col++;
            token[pos] = nch;
            pos++;
        }
    }

    token[pos] = '\0';
    // printf("tok: '%s'\n", token);

    chunk->i = i;
    chunk->col = col;
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

void read_hex(Fc *fc, char *token) {
    //
    Chunk *chunk = fc->chunk;
    char *data = chunk->content;
    int i = chunk->i;
    int col = chunk->col;
    int len = chunk->length;
    int pos = 0;
    while (i < len) {
        char ch = *(data + i);
        if (!is_hex_char(ch)) {
            break;
        }
        token[pos] = ch;
        i++;
        col++;
        pos++;
    }
    token[pos] = '\0';
    chunk->i = i;
    chunk->col = col;
}

Str *read_string(Fc *fc) {
    //
    Str *buf = fc->b->str_buf;
    str_clear(buf);

    Chunk *chunk = fc->chunk;
    char *data = chunk->content;
    int i = chunk->i;
    int col = chunk->col;
    int line = chunk->line;
    int len = chunk->length;
    while (i < len) {
        char ch = *(data + i);
        i++;
        col++;

        if (ch == '\\') {
            if (i == len) {
                break;
            }
            char add = *(data + i);
            if (add == 'n') {
                add = '\n';
            } else if (add == 'r') {
                add = '\r';
            } else if (add == 't') {
                add = '\t';
            } else if (add == 'f') {
                add = '\f';
            } else if (add == 'b') {
                add = '\b';
            } else if (add == 'v') {
                add = '\v';
            } else if (add == 'f') {
                add = '\f';
            } else if (add == 'a') {
                add = '\a';
            }
            i++;
            col++;

            str_append_char(buf, add);
            continue;
        }

        if (ch == '"') {
            break;
        }

        if (is_newline(ch)) {
            line++;
            col = 0;
        }
        str_append_char(buf, ch);
    }

    if (i == len) {
        sprintf(fc->sbuf, "Missing end of string");
        fc_error(fc);
    }

    chunk->i = i;
    chunk->col = col;
    chunk->line = line;

    return buf;
}

Array *read_string_chunks(Allocator *alc, Fc *fc) {
    //
    Array *result = array_make(alc, 4);
    Str *buf = fc->b->str_buf;
    str_clear(buf);

    Chunk *chunk = fc->chunk;
    char *data = chunk->content;
    int i = chunk->i;
    int col = chunk->col;
    int line = chunk->line;
    int len = chunk->length;
    while (i < len) {
        char ch = *(data + i);
        i++;
        col++;

        if (ch == '\\') {
            if (i == len) {
                break;
            }
            char add = *(data + i);
            if (add == 'n') {
                add = '\n';
            } else if (add == 'r') {
                add = '\r';
            } else if (add == 't') {
                add = '\t';
            } else if (add == 'f') {
                add = '\f';
            } else if (add == 'b') {
                add = '\b';
            } else if (add == 'v') {
                add = '\v';
            } else if (add == 'f') {
                add = '\f';
            } else if (add == 'a') {
                add = '\a';
            }
            i++;
            col++;

            str_append_char(buf, add);
            continue;
        }

        if (ch == '"') {
            array_push(result, str_to_chars(alc, buf));
            str_clear(buf);
            break;
        }

        if (ch == '%') {
            array_push(result, str_to_chars(alc, buf));
            str_clear(buf);
            continue;
        }

        if (is_newline(ch)) {
            line++;
            col = 1;
        }
        str_append_char(buf, ch);
    }

    if (i == len) {
        sprintf(fc->sbuf, "Missing end of string");
        fc_error(fc);
    }

    chunk->i = i;
    chunk->col = col;
    chunk->line = line;

    return result;
}

char *read_part(Allocator *alc, Fc *fc, int i, int len) {
    //
    Str *buf = fc->str_buf;
    str_clear(buf);
    Chunk *chunk = fc->chunk;
    char *data = chunk->content;
    int until = i + len;
    while (i < until) {
        char ch = data[i];
        i++;
        str_append_char(buf, ch);
    }
    return str_to_chars(alc, buf);
}
