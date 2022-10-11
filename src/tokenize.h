#pragma once

#include <stdio.h>

#include "../lib/allocator.h"
#include "../lib/dynarray.h"
#include "../lib/str.h"

#include "defines.h"
#include "types.h"

typedef struct {
  uint32_t line;
  uint32_t column;
} token_position_t;

typedef struct {
  token_position_t position;
  e_ika_type type;
  str value;
} token_t;

typedef struct {
  uint32_t pos;
  str source;
  allocator_t allocator;
  dynarray *errors;
} tokenizer_input_stream;

// An array of tokens
dynarray *tokenizer_scan(allocator_t allocator, str source, dynarray *errors);

// Debugging stuff
void tokenizer_print_token(FILE *, void *);

str tokenizer_get_token_type_label(token_t *);

str tokenizer_get_token_type_name(e_ika_type);

void tokenizer_dump_context(token_t **, uint32_t);
