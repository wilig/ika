#pragma once

#include <stdbool.h>

#include "../lib/allocator.h"
#include "../lib/dynarray.h"
#include "../lib/hashtbl.h"

#include "symtbl.h"
#include "tokenize.h"
#include "types.h"

typedef enum {
  ast_int_literal,
  ast_float_literal,
  ast_str_literal,
  ast_bool_literal,
  ast_symbol,
  ast_expr,
  ast_term,
  ast_assignment,
  ast_if_statement,
  ast_block,
  ast_fn,
  ast_decl,
  ast_return,
} e_ast_node_type;

typedef struct ast_node_t ast_node_t;

typedef struct {
  union {
    str string_value;
    uint64_t integer_value;
    float float_value;
  };
} literal_t;

typedef struct {
  str value;
} symbol_t;

typedef struct {
  e_ika_type op;
  ast_node_t *left;
  ast_node_t *right;
} expr_t;

typedef struct {
  e_ika_type op;
  ast_node_t *left;
  ast_node_t *right;
} term_t;

typedef struct {
  bool constant;
  ast_node_t *identifier;
  e_ika_type type;
  ast_node_t *expr;
} assignment_t;

typedef struct {
  ast_node_t *expr;
  ast_node_t *if_block;
  ast_node_t *else_block;
} if_t;

typedef struct {
  dynarray nodes;
} block_t;

typedef struct {
  dynarray exprs;
} return_t;

typedef struct {
  ast_node_t *identifier;
  e_ika_type type;
} decl_t;

typedef struct {
  ast_node_t *identifier;
  dynarray parameters;
  dynarray return_types;
  ast_node_t *block;
} fn_t;

typedef struct ast_node_t {
  e_ast_node_type type;
  token_t *starting_token;
  uint32_t total_tokens;
  uint32_t line, column;
  union {
    literal_t literal;
    symbol_t symbol;
    decl_t decl;
    expr_t expr;
    term_t term;
    assignment_t assignment;
    if_t if_statement;
    block_t block;
    fn_t fn;
    return_t returns;
  };
} ast_node_t;

typedef struct {
  allocator_t allocator;
  uint32_t current_token;
  dynarray *tokens;
  dynarray *errors;
} parser_state_t;

ast_node_t *parser_parse(allocator_t, dynarray *tokens, dynarray *errors);
