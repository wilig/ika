#include "format.h"
#include "allocator.h"
#include "dynarray.h"
#include "str.h"
#include <stdarg.h>
#include <stdio.h>

char *format(allocator_t allocator, const char *fmt, va_list args) {
  va_list copy;
  va_copy(copy, args);
  uint32_t length = (uint32_t)vsnprintf(NULL, 0, fmt, copy);
  char *buffer = allocator_alloc_or_exit(allocator, length);
  vsnprintf(buffer, length, fmt, args);
  return buffer;
}
