#pragma once

#include <stdio.h>

#include "../lib/allocator.h"

#include "rt/darray.h"
#include "rt/str.h"

#include "defines.h"
#include "errors.h"
#include "tokens.h"

typedef struct token_position_t {
  u32 line;
  u32 column;
} token_position_t;

typedef struct token_t {
  token_position_t position;
  e_token_type type;
  const char *value;
} token_t;

typedef struct {
  u32 pos;
  char *source;
  u64 source_length;
  da_syntax_errors *errors;
} tokenizer_input_stream_t;

// Alias to be explicit that it's a dynamic array
typedef token_t da_tokens;

da_tokens *tokenizer_scan(char *source, u64 source_length,
                          da_syntax_errors *errors);

// Debugging stuff
void tokenizer_print_token(FILE *, token_t *);

const char *tokenizer_get_token_type_label(token_t *);

const char *tokenizer_get_token_type_name(e_token_type);

void tokenizer_dump_context(token_t **, u32);
