#pragma once

#include "allocator.h"

typedef struct str {
  const char *ptr;
  uint32_t length;
} str;

str cstr(const char *);

str cstr_from_char_with_length(const char *, uint32_t);

uint32_t str_len(str);

bool str_eq(str, str);

char str_get_char(str, uint32_t);

str str_substr(str, uint32_t, uint32_t);

str str_substr_copy(allocator_t, str, uint32_t, uint32_t);

void str_copy(allocator_t allocator, str src, str *dest);

bool str_matches_at_index(str, str, uint32_t);

int str_find_idx_of_nth(uint32_t, str, str);

bool str_contains(str, str);

char *str_to_cstr(allocator_t, str);
