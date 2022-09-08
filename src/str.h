#pragma once

#include "allocator.h"
#include "defines.h"

typedef struct str {
  char *ptr;
  uint32_t length;
} str;

str cstr(char *);

str cstr_from_char_with_length(char *, int);

int str_len(str);

bool str_eq(str, str);

char str_get_char(str, uint32_t);

str str_substr(str, uint32_t, uint32_t);

str str_substr_copy(allocator_t, str, uint32_t, uint32_t);

void str_copy(allocator_t allocator, str src, str *dest);

bool str_matches_at_index(str, str, uint32_t);

int str_find_idx_of_nth(uint32_t, str, str);

bool str_contains(str, str);
