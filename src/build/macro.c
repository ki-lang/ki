
#include "../all.h"

void fc_parse_macro(FileCompiler* fc, char* token) {
  fc_next_token(fc, token, false, true, false);
  if (token[0] == '\0') {
    fc_error(fc, "Dont put a space after a '#'", NULL);
  }

  if (strcmp(token, "header") == 0) {
    // ki header
    fc_read_header_token(fc);
  } else if (strcmp(token, "if") == 0) {
    bool result = fc_resolve_macro_if_value(fc);
    array_push(fc->macro_results, result ? "1" : "0");
    if (!result) {
      fc_skip_macro(fc);
    }
  } else if (strcmp(token, "elif") == 0) {
    if (fc->macro_results->length == 0) {
      fc_error(fc, "Unexcepted else if, no previous if statement", NULL);
    }
    char* last_result = array_pop(fc->macro_results);
    bool result = fc_resolve_macro_if_value(fc);
    array_push(fc->macro_results,
               (last_result[0] == '1' || result) ? "1" : "0");
    if (last_result[0] == '1' || !result) {
      fc_skip_macro(fc);
    }
  } else if (strcmp(token, "else") == 0) {
    if (fc->macro_results->length == 0) {
      fc_error(fc, "Unexcepted else, no previous if statement", NULL);
    }
    char* last_result = array_pop(fc->macro_results);
    bool result = last_result[0] != '1';
    array_push(fc->macro_results,
               (last_result[0] == '1' || result) ? "1" : "0");
    if (!result) {
      fc_skip_macro(fc);
    }
  } else if (strcmp(token, "end") == 0) {
    if (fc->macro_results->length == 0) {
      fc_error(fc, "Unexcepted end if, no previous if statement", NULL);
    }
    char* last_result = array_pop(fc->macro_results);
  } else {
    fc_error(fc, "Unknown token: #%s", token);
  }
}

bool fc_resolve_macro_if_value(FileCompiler* fc) {
  //
  char* token = malloc(KI_TOKEN_MAX);

  fc_next_token(fc, token, false, true, true);

  if (!is_valid_varname(token)) {
    fc_error(fc, "Invalid macro variable name: '%s'", token);
  }

  char* value = map_get(macro_defines, token);
  bool result = value ? true : false;

  fc_next_token(fc, token, true, true, true);
  if (strcmp(token, "=") == 0) {
    fc_next_token(fc, token, false, true, true);
    fc_next_token(fc, token, false, true, true);
    if (result) {
      result = strcmp(value, token) == 0;
    }
  }

  fc_next_token(fc, token, true, true, true);
  if (strcmp(token, "&&") == 0) {
    fc_next_token(fc, token, false, true, true);
    result = result && fc_resolve_macro_if_value(fc);
  } else if (strcmp(token, "||") == 0) {
    fc_next_token(fc, token, false, true, true);
    result = result || fc_resolve_macro_if_value(fc);
  }

  return result;
}