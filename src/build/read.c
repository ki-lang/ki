
#include "../all.h"

Chunk *chunk_init(Allocator *alc, Fc *fc) {
    Chunk *ch = al(alc, sizeof(Chunk));
    ch->parent = NULL;
    ch->fc = fc;
    ch->content = NULL;
    ch->tokens = NULL;
    ch->length = 0;
    ch->i = 0;
    ch->line = -1;
    ch->col = -1;
    ch->token = 0;
    ch->scope_end_i = 0;

    return ch;
}
Chunk *chunk_clone(Allocator *alc, Chunk *chunk) {
    //
    Chunk *ch = al(alc, sizeof(Chunk));
    *ch = *chunk;
    return ch;
}
void chunk_move(Chunk *chunk, int pos) {
    tok_next(chunk, false, true, true);
    //
    // int i = chunk->i;
    // while (pos > 0) {
    //     pos--;
    //     char ch = chunk->content[chunk->i];
    //     chunk->i++;
    //     chunk->col++;
    //     if (ch == '\n') {
    //         chunk->line++;
    //         chunk->col = 0;
    //     }
    // }
    // while (pos < 0) {
    //     pos++;
    //     char ch = chunk->content[chunk->i];
    //     chunk->i--;
    //     chunk->col--;
    //     if (ch == '\n') {
    //         chunk->line--;
    //         int x = chunk->i;
    //         int col = 0;
    //         while (x > 0 && chunk->content[x] != '\n') {
    //             x--;
    //             col++;
    //         }
    //         chunk->col = col;
    //     }
    // }
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

char* tok(Fc *fc, bool sameline, bool allow_space) {
    //
    Chunk *chunk = fc->chunk;
    *fc->chunk_prev = *chunk;
    return tok_next(chunk, sameline, allow_space, true);
}

char* tok_next(Chunk* chunk, bool sameline, bool allow_space, bool update) {
    int x = chunk->i;
    char* res = tok_read(chunk, &x);
    char t = chunk->token;
    if(t == tok_space) {
        if(!allow_space) {
            chunk->token = tok_none;
            return "";
        }
        res = tok_read(chunk, &x);
        t = chunk->token;
    }
    if(t == tok_newline) {
        if(!allow_space || sameline){
            chunk->token = tok_none;
            return "";
        }
        res = tok_read(chunk, &x);
    }
    if(update) {
        chunk->i = x;
    }
    return res;
}

char* tok_read(Chunk* chunk, int *i_ref) {
    //
    int i = i_ref ? *i_ref : chunk->i;
    char* tokens = chunk->tokens;
    char t = tokens[i++];
    if(t == tok_pos) {
        int line = *(int*)(&tokens[i]);
        i += sizeof(int);
        int col = *(int*)(&tokens[i]);
        i += sizeof(int);
        chunk->line = line;
        chunk->col = col;
        t = tokens[i++];
    }
    if(t == tok_scope_open) {
        chunk->scope_end_i = *(int*)(&tokens[i]);
        i += sizeof(int);
    }
    char* tchars = (char*)(&tokens[i]);
    if(t != tok_eof && i_ref) {
        while(tokens[i++] != 0) {
        }
        *i_ref = i;
    }
    chunk->token = t;
    return tchars;
}

void rtok(Fc *fc) { *fc->chunk = *fc->chunk_prev; }

void tok_expect(Fc *fc, char *expect, bool sameline, bool allow_space) {
    char* token = tok(fc, sameline, allow_space);
    if (strcmp(token, expect) != 0) {
        sprintf(fc->sbuf, "Expected: '%s', but found: '%s'", expect, token);
        fc_error(fc);
    }
}

char get_char(Fc *fc, int index) {
    //
    Chunk *chunk = fc->chunk;
    char *res = tok_next(chunk, true, false, false);
    return res[0];
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

Array *string_read_format_chunks(Allocator *alc, Fc* fc, char *body) {
    //
    Array *result = array_make(alc, 4);
    Str *buf = fc->b->str_buf;
    str_clear(buf);

    int i = 0;
    while (true) {
        const char ch = body[i++];
        if(ch == 0)
            break;
        if (ch == '\\') {
            char ch = body[i++];
            ch = backslash_char(ch);
            str_append_char(buf, ch);
            continue;
        }
        if (ch == '%') {
            array_push(result, str_to_chars(alc, buf));
            str_clear(buf);
            continue;
        }
        str_append_char(buf, ch);
    }

    array_push(result, str_to_chars(alc, buf));
    str_clear(buf);

    return result;
}

char* string_replace_backslash_chars(Allocator* alc, char* body) {
    //
    int len = strlen(body);
    if(len == 0)
        return "";
    char* result = al(alc, len);
    int i = 0;
    int ri = 0;
    while(true) {
        const char ch = body[i++];
        if(ch == 0)
            break;
        if (ch == '\\') {
            char ch = body[i++];
            ch = backslash_char(ch);
            result[ri++] = ch;
            continue;
        }
        result[ri++] = ch;
    }
    result[ri] = 0;
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
