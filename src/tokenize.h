#pragma once

#include <stdio.h>

#include "../lib/allocator.h"
#include "../lib/dynarray.h"
#include "../lib/str.h"

#include "defines.h"
#include "types.h"

typedef struct token_position_t {
  u32 line;
  u32 column;
} token_position_t;

typedef struct token_t {
  token_position_t position;
  e_ika_type type;
  const char *value;
} token_t;

typedef struct {
  u32 pos;
  char *source;
  u64 source_length;
  allocator_t allocator;
  dynarray *errors;
} tokenizer_input_stream_t;

// An array of tokens
dynarray *tokenizer_scan(allocator_t allocator, char *source, u64 source_length,
                         dynarray *errors);

// Debugging stuff
void tokenizer_print_token(FILE *, token_t *);

const char *tokenizer_get_token_type_label(token_t *);

const char *tokenizer_get_token_type_name(e_ika_type);

void tokenizer_dump_context(token_t **, u32);
