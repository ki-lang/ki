
#include "all.h"

bool is_alpha_char(char c) {
    if (c >= 65 && c <= 90) {
        return true;
    }

    if (c >= 97 && c <= 122) {
        return true;
    }

    return false;
}

bool is_valid_varname_char(char c) {
    // Uppercase
    if (c >= 65 && c <= 90) {
        return true;
    }

    // Lowercase
    if (c >= 97 && c <= 122) {
        return true;
    }

    // Numbers
    if (c >= 48 && c <= 57) {
        return true;
    }

    // Underscore
    if (c == 95) {
        return true;
    }

    return false;
}
bool is_valid_varname_first_char(char c) {

    // Lowercase
    if (c >= 97 && c <= 122) {
        return true;
    }

    // Numbers
    if (c >= 48 && c <= 57) {
        return true;
    }

    // Underscore
    if (c == 95) {
        return true;
    }
    return true;
}

bool is_number(char c) {
    if (c >= 48 && c <= 57) {
        return true;
    }
    return false;
}

bool is_hex_char(char c) {
    if (c >= 48 && c <= 57) {
        return true;
    }
    // A-F
    if (c >= 65 && c <= 70) {
        return true;
    }
    // a-f
    if (c >= 97 && c <= 102) {
        return true;
    }
    return false;
}

bool is_whitespace(char c) { return c <= 32; }
bool is_newline(char c) { return c == 10; }
bool is_valid_varname(char *name) { return is_valid_varname_first_char(name[0]); }
bool is_valid_varname_all(char *name) {
    //
    int len = strlen(name);
    if (len == 0)
        return false;
    if (!is_valid_varname_first_char(name[0]))
        return false;
    int i = 0;
    while (i < len) {
        if (!is_valid_varname_char(name[i])) {
            return false;
        }
        i++;
    }
    return true;
}

bool is_valid_number(char *str) {
    int len = strlen(str);
    if (len == 0) {
        return false;
    }
    int i = 0;
    while (i < len) {
        if (!is_number(str[i])) {
            return false;
        }
        i++;
    }
    return true;
}

bool is_valid_hex_number(char *str) {
    int len = strlen(str);
    if (len == 0) {
        return false;
    }
    int i = 0;
    while (i < len) {
        char ch = str[i];
        if (!is_hex_char(ch)) {
            return false;
        }
        i++;
    }
    return true;
}

bool is_valid_macro_number(char *str) {
    int len = strlen(str);
    if (len == 0) {
        return false;
    }
    //
    char last = str[len - 1];
    if (last == 'L') {
        len--;
    }
    //
    last = str[len - 1];
    if (last == 'U') {
        len--;
    }
    //
    int i = 0;
    while (i < len) {
        if (!is_number(str[i])) {
            return false;
        }
        i++;
    }
    return true;
}

bool ends_with(const char *str, const char *suffix) {
    int str_len = strlen(str);
    int suffix_len = strlen(suffix);

    return (str_len >= suffix_len) && (0 == strcmp(str + (str_len - suffix_len), suffix));
}

bool starts_with(const char *a, const char *b) {
    if (strncmp(a, b, strlen(b)) == 0)
        return 1;
    return 0;
}

char backslash_char(char ch) {
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
