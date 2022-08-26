#pragma once

#include "allocator.h"
#include "defines.h"
#include "str.h"
#include "types.h"
#include <stdio.h>

#define TOKEN_BUCKET_SIZE 1000

typedef struct {
  uint32_t start_offset;
  e_ika_type type;
  str value;
} token;

typedef struct {
  uint32_t pos;
  str source;
  allocator_t allocator;
} tokenizer_input_stream;

token **tokenize_scan(tokenizer_input_stream *);

// Debugging stuff
void tokenize_print_token(FILE *, void *);

str tokenize_get_token_type_label(token *);

str tokenize_get_token_type_name(e_ika_type);

void tokenize_dump_context(token **, uint32_t);
