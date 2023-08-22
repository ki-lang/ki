
#include "../all.h"

void cmd_fmt_help();
void fmt_indent(Str *content, int indents);

void cmd_fmt(int argc, char *argv[]) {

    Allocator *alc = alc_make();
    Str *str_buf = str_make(alc, 5000);
    char *char_buf = al(alc, 5000);

    Array *args = array_make(alc, argc);
    Map *options = map_make(alc);
    Array *has_value = array_make(alc, 8);

    parse_argv(argv, argc, has_value, args, options);

    if (array_contains(args, "-h", arr_find_str) || array_contains(args, "--help", arr_find_str)) {
        cmd_fmt_help();
    }

    if (args->length < 3) {
        cmd_fmt_help();
    }

    Array *files = array_make(alc, argc);
    int argc_ = args->length;
    for (int i = 2; i < argc_; i++) {
        char *arg = array_get_index(args, i);
        if (arg[0] == '-') {
            continue;
        }

        char *full = al(alc, KI_PATH_MAX);
        bool success = get_fullpath(arg, full);

        if (!success || !file_exists(full)) {
            sprintf(char_buf, "fmt: file not found: '%s'", arg);
            die(char_buf);
        }

        if (!ends_with(arg, ".ki")) {
            sprintf(char_buf, "fmt: filename must end with .ki : '%s'", arg);
            die(char_buf);
        }

        array_push(files, full);
    }

    if (files->length == 0) {
        sprintf(char_buf, "Nothing to format, add some files to your fmt command");
        die(char_buf);
    }

    int filec = files->length;
    for (int i = 0; i < filec; i++) {
        char *path = array_get_index(files, i);
        fmt_format(alc, path);
    }
}

void fmt_format(Allocator *alc, char *path) {
    //
    if (!file_exists(path))
        return;
    Str *content = str_make(alc, 10000);
    file_get_contents(content, path);

    char *data = str_to_chars(alc, content);
    int len = content->length;
    str_clear(content);

    int i = 0;
    int depth = 0;
    int newlines = 0;
    int ctx = fmtc_root;
    Array *contexts = array_make(alc, 50);
    char ch = '\0';
    char pch = '\0';
    bool start_of_line = true;
    bool added_spacing = false;
    bool expects_newline = false;
    while (i < len) {
        pch = ch;
        ch = data[i];
        i++;
        // Newlines
        if (is_newline(ch)) {
            if (newlines < (depth == 0 ? 3 : 2)) {
                str_append_char(content, '\n');
                start_of_line = true;
            }
            newlines++;
            expects_newline = false;
            continue;
        }
        // Skip whitespace
        if (is_whitespace(ch)) {
            if (start_of_line) {
                continue;
            }
            if (!added_spacing) {
                str_append_char(content, ' ');
                added_spacing = true;
            }
            continue;
        }
        // Indent
        if (ch == '{' && data[i] != '{') {
            if (start_of_line) {
                fmt_indent(content, depth);
            }
            depth++;
            str_append_chars(content, "{");
            start_of_line = true;
            expects_newline = true;
            continue;
        }
        if (ch == '}') {
            if (pch != '}') {
                depth--;
                if (!start_of_line) {
                    str_append_char(content, '\n');
                }
                fmt_indent(content, depth);
            }
            str_append_chars(content, "}");
            if (data[i] != '{') {
                start_of_line = true;
                expects_newline = true;
            }
            continue;
        }
        //
        if (start_of_line && ch != ';') {
            if (expects_newline)
                str_append_char(content, '\n');
            fmt_indent(content, depth);
        }
        newlines = 0;
        start_of_line = false;
        added_spacing = false;
        //
        str_append_char(content, ch);
        // String
        if (ch == '"') {
            while (i < len) {
                ch = data[i];
                i++;
                str_append_char(content, ch);
                if (ch == '\\') {
                    str_append_char(content, data[i]);
                    i++;
                    continue;
                }
                if (ch == '"') {
                    break;
                }
            }
            continue;
        }
        // Char
        if (ch == '\'') {
            while (i < len) {
                ch = data[i];
                i++;
                str_append_char(content, ch);
                if (ch == '\\') {
                    str_append_char(content, data[i]);
                    i++;
                    continue;
                }
                if (ch == '\'') {
                    break;
                }
            }
            continue;
        }
        // Comment
        if (ch == '/' && pch == '/') {
            str_append_char(content, ' ');
            while (i < len) {
                ch = data[i];
                if (!is_whitespace(ch) || is_newline(ch))
                    break;
                i++;
            }
            while (i < len) {
                ch = data[i];
                i++;
                str_append_char(content, ch);
                if (is_newline(ch)) {
                    start_of_line = true;
                    break;
                }
            }
            continue;
        }
        if (ch == ',') {
            str_append_char(content, ' ');
            added_spacing = true;
        }
    }

    char *result = str_to_chars(alc, content);
    printf("%s\n", result);
}

void fmt_indent(Str *content, int indents) {
    for (int i = 0; i < indents; i++) {
        str_append_chars(content, "    ");
    }
}

void cmd_fmt_help() {
    //
    printf("> ki fmt {ki file paths}\n");
    printf("\n");

    exit(1);
}
