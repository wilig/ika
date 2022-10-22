#include "helpers.h"

#include <string.h>

#define MAX_STRING_LEN 10000

bool streq(const char *s, const char *s1) {
  u64 len = strnlen(s, MAX_STRING_LEN);
  if (len != strnlen(s1, MAX_STRING_LEN))
    return false;
  return strncmp(s, s1, len) == 0;
}

bool streq_n(const char *s, const char *s1, u32 n) {
  u64 len = strnlen(s, n);
  if (len != strnlen(s1, n))
    return false;
  return strncmp(s, s1, n) == 0;
}
