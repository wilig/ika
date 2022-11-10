#pragma once

#include "../compiler.h"
#include "../rt/str_builder.h"

typedef struct c11_be_t {
  str_builder_t *sb;
  u32 ident_level;
  char *filename;
} c11_be_t;

static char *c11_op_codes[TOKEN_EOF] = {
    [TOKEN_INT] = "i64",     [TOKEN_INT_LITERAL] = "i64",
    [TOKEN_BOOL] = "b8",     [TOKEN_BOOL_LITERAL] = "b8",
    [TOKEN_FLOAT] = "f64",   [TOKEN_FLOAT_LITERAL] = "f64",
    [TOKEN_STR] = "str",     [TOKEN_STR_LITERAL] = "str",
    [TOKEN_RUNE] = "char *", [TOKEN_VOID] = "void",
    [TOKEN_ANY] = "any_t",   [TOKEN_ADD] = "+",
    [TOKEN_SUB] = "-",       [TOKEN_MUL] = "*",
    [TOKEN_QUO] = "/",       [TOKEN_ASSIGN] = "=",
    [TOKEN_EQL] = "==",      [TOKEN_NEQ] = "!=",
    [TOKEN_LT] = "<",        [TOKEN_GT] = ">",
    [TOKEN_LTE] = "<=",      [TOKEN_GTE] = ">=",
    [TOKEN_MOD] = "%"};

b8 c11_generate(compilation_unit_t *);
