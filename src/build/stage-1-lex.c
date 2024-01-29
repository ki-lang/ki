
#include "../all.h"

void stage_1_lex(Fc *fc) {
    //
    Chunk *chunk = fc->chunk;
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


    Str* tokens_str = str_make(fc->alc, length * 2 + 512);
    char* tokens = tokens_str->data;

    while(true) {
        const char ch = content[i];
        if(ch == '\0') 
            break;
        i++;
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
            char ch = content[i];
            while(ch <= 32) {
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
                sprintf(fc->sbuf, "Unexpected closing tag '%c'", ch);
                fc_error(fc);
            }
            if(closer_chars[depth] != ch) {
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

        sprintf(fc->sbuf, "Unexpected token '%c'", ch);
        fc_error(fc);
    }

    if(depth > 0) {
        sprintf(fc->sbuf, "Missing closing tag '%c'", closer_chars[depth - 1]);
        fc_error(fc);
    }
}