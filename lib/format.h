#pragma once

#include <stdarg.h>

#include "allocator.h"

char *format(allocator_t, const char *, va_list);
