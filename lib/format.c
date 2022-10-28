#include "format.h"
#include "allocator.h"
#include "str.h"
#include <stdarg.h>
#include <stdio.h>

char *format(const char *fmt, va_list args) {
  va_list copy;
  va_copy(copy, args);
  uint32_t length = (uint32_t)vsnprintf(NULL, 0, fmt, copy);
  char *buffer = imust_alloc(length);
  vsnprintf(buffer, length, fmt, args);
  return buffer;
}
