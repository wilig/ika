#pragma once

#include "../defines.h"

#include "str.h"

void _ika_print_int(i64);
void _ika_print_float(f64);
void _ika_print_bool(b8);
void _ika_print_str(str);

#define print(T)                                                               \
  _Generic((T), str                                                            \
           : _ika_print_str, i64                                               \
           : _ika_print_int, f64                                               \
           : _ika_print_float, b8                                              \
           : _ika_print_bool)(T);
