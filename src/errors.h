#pragma once

#include "../lib/dynarray.h"
#include "../lib/str.h"

#include "defines.h"

typedef enum {
  SUCCESS,
  ERROR_VARIABLE_REDEFINITION,
} IKA_STATUS;

typedef enum { TOKENIZE, PARSING, TYPING, IR_GEN } IKA_PASS;

typedef struct {
  IKA_PASS pass;
  uint32_t line;
  uint32_t column;
  char *message;
} syntax_error_t;

void errors_display_parser_errors(dynarray *errors, char *source);
