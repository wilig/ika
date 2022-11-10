#include "print.h"

#include <stdio.h>

void _ika_print_int(i64 v) { printf("%li", v); }

void _ika_print_float(f64 v) { printf("%f", v); }

void _ika_print_bool(b8 v) { printf("%s", v == 0 ? "false" : "true"); }

void _ika_print_str(str v) { printf("%.*s", (int)v.length, v.ptr); }
