#pragma once

#include <stdbool.h>

#include "../lib/allocator.h"
#include "../lib/dynarray.h"
#include "../lib/hashtbl.h"

#include "../lib/allocator.h"
#include "../lib/dynarray.h"

#include "ast.h"
#include "symbol_table.h"
#include "tokenize.h"

typedef struct {
  allocator_t allocator;
  uint32_t current_token;
  symbol_table_t *current_scope;
  dynarray *tokens;
  dynarray *errors;
} parser_state_t;

ast_node_t *parser_parse(allocator_t, dynarray *tokens, dynarray *errors);
