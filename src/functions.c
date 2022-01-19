
#include "all.h"

void die(char *msg) {
  printf("Error: %s\n", msg);
  exit(1);
}

bool ends_with(const char *str, const char *suffix) {
  int str_len = strlen(str);
  int suffix_len = strlen(suffix);

  return (str_len >= suffix_len) &&
         (0 == strcmp(str + (str_len - suffix_len), suffix));
}

char *rand_string(char *str, int size) {
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMOPQRSTUVWXYZ";
  if (size) {
    --size;
    for (int n = 0; n < size; n++) {
      int key = rand() % (int)(sizeof charset - 1);
      str[n] = charset[key];
    }
    str[size] = '\0';
  }
  return str;
}

int atoi(const char *str) {
  int num = 0;
  int i = 0;
  bool isNegetive = false;
  if (str[i] == '-') {
    isNegetive = true;
    i++;
  }
  while (str[i] && (str[i] >= '0' && str[i] <= '9')) {
    num = num * 10 + (str[i] - '0');
    i++;
  }
  if (isNegetive) num = -1 * num;
  return num;
}

void prepend(char *s, const char *t) {
  size_t len = strlen(t);
  memmove(s + len, s, strlen(s) + 1);
  memcpy(s, t, len);
}
