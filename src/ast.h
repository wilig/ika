#pragma once

#include "symbol_table.h"
#include "tokenize.h"
#include "types.h"

typedef enum {
  ast_int_literal = 0,
  ast_float_literal = 1,
  ast_str_literal = 2,
  ast_bool_literal = 3,
  ast_symbol = 4,
  ast_expr = 5,
  ast_term = 6,
  ast_assignment = 7,
  ast_print_stmt = 8,
  ast_if_stmt = 9,
  ast_block = 10,
  ast_fn = 11,
  ast_fn_call = 12,
  ast_decl = 13,
  ast_return = 14,
} e_ast_node_type;

typedef struct ast_node_t ast_node_t;

typedef struct ast_node_t da_nodes;

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
  ast_node_t *symbol;
  ast_node_t *expr;
} assignment_t;

typedef struct {
  ast_node_t *expr;
} print_t;

typedef struct {
  ast_node_t *expr;
  ast_node_t *if_block;
  ast_node_t *else_block;
} if_t;

typedef struct {
  ast_node_t *expr;
} return_t;

typedef struct block_t {
  da_nodes *nodes;
  symbol_table_t *symbol_table;
  ast_node_t *return_statement;
} block_t;

typedef struct {
  b8 constant;
  ast_node_t *symbol;
  e_ika_type type;
  ast_node_t *expr;
} decl_t;

typedef struct {
  ast_node_t *symbol;
  da_nodes *parameters;
  symbol_table_t *parameters_symbol_table;
  e_ika_type return_type;
  ast_node_t *block;
} fn_t;

typedef struct fn_call_t {
  ast_node_t *symbol;
  da_nodes *exprs;
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
    print_t print_stmt;
    if_t if_stmt;
    block_t block;
    fn_t fn;
    fn_call_t fn_call;
    return_t returns;
  };
} ast_node_t;
