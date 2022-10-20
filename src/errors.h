#pragma once

#include "../lib/dynarray.h"
#include "../lib/str.h"

typedef enum {
  SUCCESS,
  VARIABLE_REDEFINITION_ERROR,
} IKA_ERROR;

typedef enum {
  tokenizing_pass,
  parsing_pass,
  typing_pass,
  ir_pass
} compiler_pass;

typedef struct {
  compiler_pass pass;
  uint32_t line;
  uint32_t column;
  str message;
  str hint;
} syntax_error_t;

void errors_display_parser_errors(dynarray *errors, str source);
