#pragma once

#include "../lib/dynarray.h"
#include "../lib/str.h"

typedef enum {
  tokenizing_pass,
  parsing_pass,
  analysis_pass,
  typing_pass,
  ir_pass
} compiler_pass;

typedef struct {
  compiler_pass pass;
  int line;
  int column;
  str message;
  str hint;
} syntax_error_t;

void errors_display_parser_errors(dynarray *errors, str source);
