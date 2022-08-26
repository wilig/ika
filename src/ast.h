#pragma once

#include "hashtbl.h"
#include "tokenize.h"
#include "types.h"
#include <stdint.h>

typedef struct stmt_t stmt_t;
typedef struct expr_t expr_t;
typedef struct scope_t scope_t;

typedef enum e_expr_type {
  literal_value,
  identifier,
  binary_expr,
} e_expr_type;

typedef struct literal_value_t {
  // Should be a single token;
  int start_token;
  int end_token;

  e_ika_type type;
  token *value;
} literal_value_t;

typedef struct identifier_t {
  // Should be a single token;
  int start_token;
  int end_token;

  e_ika_type type;
  token *value;
} identifier_t;

typedef struct binary_expr_t {
  int start_token;
  int end_token;

  expr_t *left;
  token *op;
  expr_t *right;
} binary_expr_t;

typedef struct expr_t {
  e_expr_type type;
  union {
    literal_value_t literal;
    identifier_t identifier;
    binary_expr_t binary;
  };
} expr_t;

typedef enum e_stmt_type {
  let_statement,
  import_statement,
  namespace_statement,
  var_statement,
  assignment_statement,
  if_statement,
} e_stmt_type;

typedef struct assignment_stmt_t {
  int start_token;
  int end_token;
  token *identifier;
  token *op;
  expr_t *expr;
} assignment_stmt_t;

typedef struct var_stmt_t {
  int start_token;
  int end_token;
  token *identifier;
  token *explicit_type;
  expr_t *expr;
} var_stmt_t;

typedef struct import_stmt_t {
  int start_token;
  int end_token;

  token *pathTokens;
} import_stmt_t;

typedef struct namespace_stmt_t {
  int start_token;
  int end_token;

  token *nameToken;
} namespace_stmt_t;

typedef struct let_stmt_t {
  int start_token;
  int end_token;
  token *identifier;
  token *explicit_type;
  expr_t *expr;
} let_stmt_t;

typedef struct if_stmt_t {
  int start_token;
  int end_token;
  expr_t *conditional;
  scope_t *if_scope;
  scope_t *else_scope;
} if_stmt_t;

typedef struct stmt_t {
  e_stmt_type type;
  union {
    import_stmt_t import_statement;
    namespace_stmt_t namespace_statement;
    let_stmt_t let_statement;
    var_stmt_t var_statement;
    assignment_stmt_t assignment_statement;
    if_stmt_t if_statement;
  };
} stmt_t;

typedef struct scope_t {
  allocator_t allocator;
  str name;
  stmt_t **decls;
  int total_decls;
  hashtbl_str_t symbol_table;
  struct scope_t *parent;
  struct scope_t **children;
  size_t number_of_children;
} scope_t;
