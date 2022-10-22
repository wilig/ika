#pragma once

#include "symbol_table.h"
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
  ast_fn_call,
  ast_decl,
  ast_return,
} e_ast_node_type;

typedef struct ast_node_t ast_node_t;

typedef struct {
  union {
    const char *string_value;
    i64 integer_value;
    f64 float_value;
  };
} literal_t;

typedef struct {
  char *value;
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
  ast_node_t *symbol;
  e_ika_type type;
  ast_node_t *expr;
} assignment_t;

typedef struct {
  ast_node_t *expr;
  ast_node_t *if_block;
  ast_node_t *else_block;
} if_t;

typedef struct {
  ast_node_t *expr;
} return_t;

typedef struct block_t {
  dynarray nodes; // type: ast_node_t
  symbol_table_t *symbol_table;
  ast_node_t *return_statement;
} block_t;

typedef struct {
  ast_node_t *symbol;
  e_ika_type type;
} decl_t;

typedef struct {
  ast_node_t *symbol;
  dynarray parameters;
  symbol_table_t *parameters_symbol_table;
  e_ika_type return_type;
  ast_node_t *block;
} fn_t;

typedef struct {
  ast_node_t *symbol;
  dynarray exprs;
} fn_call_t;

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
    fn_call_t fn_call;
    return_t returns;
  };
} ast_node_t;
