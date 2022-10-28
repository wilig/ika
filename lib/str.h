#pragma once

#include "allocator.h"

typedef struct str {
  const char *ptr;
  u32 length;
} str;

str cstr(const char *);

str cstr_from_char_with_length(const char *, u32);

uint32_t str_len(str);

b8 str_eq(str, str);

char str_get_char(str, u32);

str str_substr(str, u32, u32);

str str_substr_copy(str, u32, u32);

void str_copy(str src, str *dest);

b8 str_matches_at_index(str, str, u32);

int str_find_idx_of_nth(u32, str, str);

b8 str_contains(str, str);

char *str_to_cstr(str);
