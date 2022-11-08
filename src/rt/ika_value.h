#pragma once

#include "../defines.h"
#include "str.h"

typedef enum e_ika_value_type { INTEGER, FLOAT, BOOL, STRING } e_ika_value_type;

typedef struct ika_value_t {
  e_ika_value_type itype;
  union {
    u64 integer;
    f64 flt;
    b8 bool;
    str string;
  } as;
} ika_value_t;
