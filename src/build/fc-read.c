
#include "../all.h"

char fc_get_char(FileCompiler *fc, int offset) {
  int sum = fc->i + offset;
  if (sum >= fc->content_len) {
    return '\0';
  }
  return fc->content[sum];
}

void fc_next_token(FileCompiler *fc, char *token, bool readonly, bool sameline,
                   bool allow_space) {
  int ti = 0;
  int index = fc->i;
  bool is_num = false;
  while (index < fc->content_len) {
    char ch = fc->content[index];

    if (!allow_space && (is_newline(ch) || is_whitespace(ch))) {
      break;
    }

    if (is_newline(ch)) {
      if (!readonly) {
        LOC++;
      }
      if (!sameline && ti == 0) {
        index++;
        continue;
      }
      break;
    }

    if (is_whitespace(ch) && ti == 0) {
      index++;
      continue;
    }

    if (ti == 0 && is_number(ch)) {
      is_num = true;
      token[ti] = ch;
      index++;
      ti++;
      continue;
    }

    if (is_num) {
      if (is_number(ch)) {
        token[ti] = ch;
        index++;
        ti++;
        continue;
      }
      break;
    }

    if (is_valid_varname_char(ch) || (ti > 0 && is_number(ch))) {
      token[ti] = ch;
      ti++;
      index++;
      continue;
    }

    if (ti == 0) {
      char nch = '\0';
      if (index < fc->content_len) {
        nch = fc->content[index + 1];
      }

      if (ch == '/' && nch == '/') {
        int prev = fc->i;
        fc->i = index;
        fc_skip_comment(fc);
        index = fc->i;
        fc->i = prev;
        continue;
      }

      token[ti] = ch;
      ti++;
      index++;

      if ((ch == ':' && nch == '=') || (ch == '=' && nch == '=') ||
          (ch == '!' && nch == '=') || (ch == '<' && nch == '=') ||
          (ch == '>' && nch == '=') || (ch == '+' && nch == '+') ||
          (ch == '-' && nch == '-') || (ch == '+' && nch == '=') ||
          (ch == '-' && nch == '=') || (ch == '*' && nch == '=') ||
          (ch == '&' && nch == '&') || (ch == '|' && nch == '|') ||
          (ch == '/' && nch == '=') || (ch == '<' && nch == '<') ||
          (ch == '>' && nch == '>')) {
        token[ti] = nch;
        ti++;
        index++;
      }
    }

    break;
  }

  token[ti] = '\0';

  if (!readonly) {
    fc->i = index;
  }
}

void fc_expect_token(FileCompiler *fc, char *ch, bool readonly, bool sameline,
                     bool allow_space) {
  char *token = malloc(KI_TOKEN_MAX);
  fc_next_token(fc, token, readonly, sameline, allow_space);
  if (strcmp(token, ch) != 0) {
    char *msg = malloc(512);
    sprintf(msg, "Expected \"%s\" but found: \"%s\"", ch, token);
    fc_error(fc, msg, token);
  }
  free(token);
}
