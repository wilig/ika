#include "str.h"

#include <stdio.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

str str_new(const char *c_chars, u64 length) {
  char *new_storage = imust_alloc(length);
  memcpy(new_storage, c_chars, length);
  return (str){.ptr = new_storage, .length = length};
}

str cstr(const char *raw_chars) {
  uint32_t length = 0;
  while (raw_chars[length] != '\0')
    length++;
  return (str){.ptr = raw_chars, .length = length};
}

str cstr_from_char_with_length(const char *raw_chars, uint32_t length) {
  return (str){.ptr = raw_chars, .length = (uint32_t)length};
}

u64 str_len(str s) { return s.length; }

b8 str_eq(str s1, str s2) {
  if (s1.length != s2.length)
    return FALSE;
  for (u64 i = 0; i < s1.length; i++) {
    if (s1.ptr[i] != s2.ptr[i])
      return FALSE;
  }
  return TRUE;
}

str str_substr(str s, u64 starting_idx, u64 length) {
  return (str){.ptr = &s.ptr[starting_idx], .length = length};
}

str str_substr_copy(str s, u64 starting_idx, u64 length) {
  void *mem = imust_alloc(sizeof(uint8_t) * length);
  memcpy(mem, &s.ptr[starting_idx], length);
  return (str){.ptr = mem, .length = length};
}

void str_copy(str src, str *dest) {
  char *copy = imust_alloc(sizeof(char) * src.length);
  memcpy(copy, src.ptr, src.length);
  dest->ptr = copy;
  dest->length = src.length;
}

char str_get_char(str s, u64 pos) {
  if (pos < s.length) {
    return s.ptr[pos];
  }
  return 0;
}

b8 str_matches_at_index(str haystack, str needle, u64 position) {
  if (position + needle.length > haystack.length) {
    return FALSE;
  }
  for (int i = 0; i < needle.length; i++) {
    if (haystack.ptr[position + i] != needle.ptr[i]) {
      return FALSE;
    }
  }
  return TRUE;
}

i64 str_find_idx_of_nth(u32 nth, str haystack, str needle) {
  u32 match = 0;
  for (u64 i = 0; i < haystack.length; i++) {
    if (haystack.ptr[i] == needle.ptr[0]) {
      if (str_eq(str_substr(haystack, i, needle.length), needle)) {
        if (match == nth - 1) {
          return (int)i;
        } else {
          match++;
        }
      }
    }
  }
  return -1;
}

b8 str_contains(str haystack, str needle) {
  return str_find_idx_of_nth(1, haystack, needle) != -1;
}

char *str_to_cstr(str value) {
  char *new_str = imust_alloc(value.length + 1); // Add room for the trailing \0
  return strncpy(new_str, (char *)value.ptr, value.length);
}
