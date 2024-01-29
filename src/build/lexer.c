
#include "../all.h"

void chunk_lex(Fc *fc, Chunk* chunk, int err_i) {
    //
    char* content = chunk->content;
    int length = chunk->length;

    int i = 0;
    int o = 0;
    int depth = 0;
    char closer_chars[256];
    int closer_indexes[256];
    char bracket_table[128];
    bracket_table['('] = ')';
    bracket_table['['] = ']';
    bracket_table['{'] = '}';

    int line = 0;
    int col = 0;
    int i_last = 0;

    Str* tokens_str = str_make(fc->alc, length * 2 + 512);
    char* tokens = tokens_str->data;

    while(true) {
        const char ch = content[i];
        if(err_i > -1 && i >= err_i) {
            // Throw err
        }
        if(ch == '\0') 
            break;
        i++;
        col += i - i_last;
        i_last = i;
        // Make sure we have enough memory
        if(tokens_str->mem_size - o < 512) {
            tokens_str->length = o;
            str_increase_memsize(tokens_str, tokens_str->mem_size * 2);
            tokens = tokens_str->data;
        }
        if(ch == ' ' || ch == '\t') {
            tokens[o++] = tok_space;
            tokens[o++] = 0;
            char ch = content[i];
            while(ch == ' ' || ch == '\t') {
                ch = content[++i];
            }
            continue;
        }
        if(ch == '\n') {
            tokens[o++] = tok_newline;
            tokens[o++] = 0;
            i_last = i;
            col = 0;
            line++;
            char ch = content[i];
            while(ch <= 32) {
                if(ch == 0)
                    break;
                ch = content[++i];
            }
            continue;
        }
        if(ch == '\r') {
            continue;
        }
        // ID: a-zA-Z_
        if((ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122) || ch == 95 || ch == '@') {
            tokens[o++] = ch == '@' ? tok_at_word : tok_id;
            tokens[o++] = ch;
            // a-zA-Z0-9_
            char ch = content[i];
            while ((ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122) || (ch >= 48 && ch <= 57) || ch == 95) {
                tokens[o++] = ch;
                ch = content[++i];
            }
            tokens[o++] = '\0';
            continue;
        }
        // Number
        if (ch >= 48 && ch <= 57) {
            tokens[o++] = tok_number;
            tokens[o++] = ch;
            // a-zA-Z0-9_
            char ch = content[i];
            while (ch >= 48 && ch <= 57) {
                tokens[o++] = ch;
                ch = content[++i];
            }
            tokens[o++] = '\0';
            continue;
        }
        // Comments
        if(ch == '/' && content[i] == '/') {
            i++;
            char ch = content[i];
            while (ch != '\n' && ch != 0) {
                ch = content[++i];
            }
            if(ch == '\n')
                i++;
            continue;
        }
        // Strings
        if(ch == '"') {
            tokens[o++] = tok_string;
            char ch = content[i];
            while(ch != '"') {
                tokens[o++] = ch;
                if(ch == '\\') {
                    ch = content[++i];
                    tokens[o++] = ch;
                }
                if(ch == 0){
                    sprintf(fc->sbuf, "Missing string closing tag '\"', compiler reached end of file");
                    fc_error(fc);
                }
                ch = content[++i];
                // Extend memory if needed
                if ((o % 200 == 0) && tokens_str->mem_size - o < 512) {
                    tokens_str->length = o;
                    str_increase_memsize(tokens_str, tokens_str->mem_size * 2);
                    tokens = tokens_str->data;
                }
            }
            tokens[o++] = '\0';
            i++;
            continue;
        }
        if(ch == '\'') {
            char ch = content[i++];
            if(ch == '\\') {
                ch = content[i++];
                ch = convert_backslash_char(ch);
            }
            if(content[i++] != '\'') {
                sprintf(fc->sbuf, "Missing character closing tag ('), found '%c'", content[i - 1]);
                fc_error(fc);
            }
            tokens[o++] = tok_char_string;
            tokens[o++] = ch;
            tokens[o++] = '\0';
            continue;
        }
        // Scopes
        if(ch == '(' || ch == '[' || ch == '{') {
            tokens[o++] = tok_scope_open;
            int index = o;
            o += sizeof(int);
            tokens[o++] = ch;
            tokens[o++] = '\0';
            closer_chars[depth] = bracket_table[ch];
            closer_indexes[depth] = index;
            depth++;
            continue;
        }
        if(ch == ')' || ch == ']' || ch == '}') {
            depth--;
            if(depth < 0) {
                chunk->i = i;
                sprintf(fc->sbuf, "Unexpected closing tag '%c'", ch);
                fc_error(fc);
            }
            if(closer_chars[depth] != ch) {
                chunk->i = i;
                sprintf(fc->sbuf, "Unexpected closing tag '%c', expected '%c'", ch, closer_chars[depth]);
                fc_error(fc);
            }
            tokens[o++] = tok_scope_close;
            tokens[o++] = ch;
            tokens[o++] = '\0';
            int offset = closer_indexes[depth];
            *(int*)((intptr_t)tokens + offset) = o;
            continue;
        }
        // Operators
        bool op2 = false;
        if (ch == '=' || ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '!' || ch == '<' || ch == '>') {
            const char ch2 = content[i];
            if(ch2 == '=')
                op2 = true;
            else if((ch == '+' && ch2 == '+') || (ch == '-' && ch2 == '-') || (ch == '!' && ch2 == '!') || (ch == '!' && ch2 == '?') || (ch == '>' && ch2 == '>') || (ch == '<' && ch2 == '<')) {
                if(ch == '-' && content[i + 1] == '-') {
                    tokens[o++] = tok_op3;
                    tokens[o++] = ch;
                    tokens[o++] = ch;
                    tokens[o++] = ch;
                    tokens[o++] = '\0';
                    i += 2;
                    continue;
                }
                op2 = true;
            }
        } else if(ch == '?') {
            const char ch2 = content[i];
            if(ch2 == '?')
                op2 = true;
            else if(ch2 == '!')
                op2 = true;
        } else if(ch == '&' && content[i] == '&') {
            op2 = true;
        } else if(ch == '|' && content[i] == '|') {
            op2 = true;
        }
        if(op2) {
            tokens[o++] = tok_op2;
            tokens[o++] = ch;
            tokens[o++] = content[i];
            tokens[o++] = '\0';
            i++;
            continue;
        }
        if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '<' || ch == '>' || ch == '%' || ch == '^' || ch == '|') {
            tokens[o++] = tok_op1;
            tokens[o++] = ch;
            tokens[o++] = '\0';
            continue;
        }
        if (ch == '!' || ch == '=' || ch == '&' || ch == ':' || ch == '?' || ch == '.' || ch == '~' || ch == '#' || ch == ';' || ch == ',') {
            tokens[o++] = tok_char;
            tokens[o++] = ch;
            tokens[o++] = '\0';
            continue;
        }

        chunk->i = i;
        printf("i: %d\n", i);
        printf("len: %d\n", chunk->length);
        sprintf(fc->sbuf, "Unexpected token '%c'", ch);
        fc_error(fc);
    }

    if(depth > 0) {
        sprintf(fc->sbuf, "Missing closing tag '%c'", closer_chars[depth - 1]);
        fc_error(fc);
    }

    tokens[o++] = tok_eof;
    tokens[o++] = '\0';

    chunk->tokens= tokens;
}

char convert_backslash_char(char ch) {
    if (ch == 'n') {
        ch = '\n';
    } else if (ch == 'r') {
        ch = '\r';
    } else if (ch == 't') {
        ch = '\t';
    } else if (ch == 'f') {
        ch = '\f';
    } else if (ch == 'b') {
        ch = '\b';
    } else if (ch == 'v') {
        ch = '\v';
    } else if (ch == 'f') {
        ch = '\f';
    } else if (ch == 'a') {
        ch = '\a';
    }
    return ch;
}