
#include "../all.h"

bool fc_resolve_if_value(FileCompiler* fc) {
  //
  char* token = malloc(KI_TOKEN_MAX);
  fc_next_token(fc, token, false, true, true);

  if (!is_valid_varname(token)) {
    fc_error(fc, "Invalid macro variable name: '%s'", token);
  }

  fc_next_token(fc, token, false, true, true);
  if (token[0] != '\0') {
    // todo
  }

  return true;
}