
#include "../all.h"

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