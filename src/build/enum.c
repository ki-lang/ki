
#include "../all.h"

Enum* init_enum() {
  Enum* enu = malloc(sizeof(Enum));
  enu->name = NULL;
  enu->values = map_make();
  return enu;
}

void free_enum(Enum* enu) {
  //
  free(enu);
}

void fc_read_enum_values(FileCompiler* fc, Enum* enu) {
  char* token = malloc(KI_TOKEN_MAX);

  int auto_value = 0;
  fc_expect_token(fc, "{", false, true, true);
  fc_next_token(fc, token, false, false, true);
  while (strcmp(token, "}") != 0) {
    if (!is_valid_varname(token)) {
      fc_error(fc, "Invalid enum property name: '%s'", token);
    }

    char* name = strdup(token);
    fc_next_token(fc, token, false, true, true);

    if (strcmp(token, ":") == 0) {
      // Custom value
      fc_next_token(fc, token, false, true, true);
      if (!is_valid_number(token)) {
        fc_error(fc, "Invalid enum value: '%s'", token);
      }
      map_set(enu->values, name, strdup(token));
      fc_next_token(fc, token, false, false, true);
    } else {
      // Auto value
      char* vc = malloc(sizeof(int) + 1);
      sprintf(vc, "%d", auto_value);
      map_set(enu->values, name, vc);
      auto_value++;
      // Check if token was empty, get token next line
      if (token[0] == '\0') {
        fc_next_token(fc, token, false, false, true);
      }
    }

    if (strcmp(token, ",") != 0 && strcmp(token, "}") != 0) {
      fc_error(fc, "Expected either ',' or '}' instead of: '%s'", token);
    }
    if (strcmp(token, ",") == 0) {
      fc_next_token(fc, token, false, false, true);
    }
  }

  free(token);
}
