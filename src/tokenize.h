#pragma once

#include "allocator.h"
#include "defines.h"
#include "str.h"
#include "types.h"

#include <stdio.h>

#define TOKEN_BUCKET_SIZE 1000

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
} tokenizer_input_stream;

token_t **tokenizer_scan(tokenizer_input_stream *);

// Debugging stuff
void tokenizer_print_token(FILE *, void *);

str tokenizer_get_token_type_label(token_t *);

str tokenizer_get_token_type_name(e_ika_type);

void tokenizer_dump_context(token_t **, uint32_t);
