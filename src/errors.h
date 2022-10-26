#pragma once

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

// Alias to be explicit that it's a dynamic array
typedef syntax_error_t da_syntax_errors;

void errors_display_parser_errors(da_syntax_errors *errors, char *source);
