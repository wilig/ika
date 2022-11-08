#pragma once

#include "../../lib/allocator.h"
#include "../defines.h"

typedef struct str {
  u64 length;
  const char *ptr;
} str;

str str_new(const char *, u64 length);

str cstr(const char *);

str cstr_from_char_with_length(const char *, u32);

u64 str_len(str);

b8 str_eq(str, str);

char str_get_char(str, u64);

str str_substr(str, u64, u64);

str str_substr_copy(str, u64, u64);

void str_copy(str src, str *dest);

b8 str_matches_at_index(str, str, u64);

i64 str_find_idx_of_nth(u32, str, str);

b8 str_contains(str, str);

char *str_to_cstr(str);
