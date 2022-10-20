#include "str.h"
#include "allocator.h"
#include "log.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

str cstr(const char *raw_chars) {
  uint32_t length = 0;
  while (raw_chars[length] != '\0')
    length++;
  return (str){.ptr = raw_chars, .length = length};
}

str cstr_from_char_with_length(const char *raw_chars, uint32_t length) {
  return (str){.ptr = raw_chars, .length = (uint32_t)length};
}

uint32_t str_len(str s) { return s.length; }

bool str_eq(str s1, str s2) {
  if (s1.length != s2.length)
    return false;
  for (uint32_t i = 0; i < s1.length; i++) {
    if (s1.ptr[i] != s2.ptr[i])
      return false;
  }
  return true;
}

str str_substr(str s, uint32_t starting_idx, uint32_t length) {
  return (str){.ptr = &s.ptr[starting_idx], .length = length};
}

str str_substr_copy(allocator_t allocator, str s, uint32_t starting_idx,
                    uint32_t length) {
  allocated_memory mem = allocator_alloc(allocator, sizeof(uint8_t) * length);
  if (!mem.valid) {
    log_error("Couldn't allocate memory for str_substr_copy");
    exit(-1);
  }
  memcpy(mem.ptr, &s.ptr[starting_idx], length);
  return (str){.ptr = mem.ptr, .length = length};
}

void str_copy(allocator_t allocator, str src, str *dest) {
  char *copy = allocator_alloc_or_exit(allocator, sizeof(char) * src.length);
  memcpy(copy, src.ptr, src.length);
  dest->ptr = copy;
  dest->length = src.length;
}

char str_get_char(str s, uint32_t pos) {
  if (pos < s.length) {
    return s.ptr[pos];
  }
  return 0;
}

bool str_matches_at_index(str haystack, str needle, uint32_t position) {
  if (position + needle.length > haystack.length) {
    return false;
  }
  for (int i = 0; i < needle.length; i++) {
    if (haystack.ptr[position + i] != needle.ptr[i]) {
      return false;
    }
  }
  return true;
}

int str_find_idx_of_nth(uint32_t nth, str haystack, str needle) {
  uint32_t match = 0;
  for (uint32_t i = 0; i < haystack.length; i++) {
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

bool str_contains(str haystack, str needle) {
  return str_find_idx_of_nth(1, haystack, needle) != -1;
}

char *str_to_cstr(allocator_t allocator, str value) {
  char *new_str = allocator_alloc_or_exit(
      allocator, value.length + 1); // Add room for the trailing \0
  return strncpy(new_str, value.ptr, value.length);
}
