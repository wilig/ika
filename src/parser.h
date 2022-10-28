#pragma once

#include <stdbool.h>

#include "../lib/allocator.h"
#include "../lib/darray.h"
#include "../lib/hashtbl.h"

#include "ast.h"
#include "errors.h"
#include "symbol_table.h"
#include "tokenize.h"

typedef struct {
  u32 current_token;
  symbol_table_t *current_scope;
  da_tokens *tokens;
  da_syntax_errors *errors;
} parser_state_t;

ast_node_t *parser_parse(da_tokens *tokens, da_syntax_errors *errors);
