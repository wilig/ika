#pragma once

#include "../compiler.h"
#include "../rt/str_builder.h"

typedef struct c11_be_t {
  str_builder_t *sb;
  u32 ident_level;
  char *filename;
} c11_be_t;

static char *c11_op_codes[ika_eof] = {
    [ika_int] = "i64",     [ika_int_literal] = "i64",
    [ika_bool] = "b8",     [ika_bool_literal] = "b8",
    [ika_float] = "f64",   [ika_float_literal] = "f64",
    [ika_str] = "str",     [ika_str_literal] = "str",
    [ika_rune] = "char *", [ika_void] = "void",
    [ika_any] = "any_t",   [ika_add] = "+",
    [ika_sub] = "-",       [ika_mul] = "*",
    [ika_quo] = "/",       [ika_assign] = "=",
    [ika_eql] = "==",      [ika_neq] = "!=",
    [ika_lt] = "<",        [ika_gt] = ">",
    [ika_lte] = "<=",      [ika_gte] = ">=",
};

b8 c11_generate(compilation_unit_t *);
