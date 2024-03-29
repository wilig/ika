#include <assert.h>

#include "../lib/allocator.h"
#include "../lib/format.h"

#include "rt/darray.h"

#include "ast.h"
#include "errors.h"
#include "helpers.h"
#include "parser.h"
#include "symbol_table.h"
#include "tokenize.h"
#include "types.h"

static void parse_error(parser_state_t *state, u32 line, u32 column,
                        const char *fmt, ...) {
  syntax_error_t err = {.line = line, .column = column, .pass = PARSING};
  va_list args;
  va_start(args, fmt);
  err.message = format(fmt, args);
  darray_append(state->errors, err);
}

ast_node_t *parse_node(parser_state_t *);
ast_node_t *parse_expr(parser_state_t *);

static void advance_token_pointer(parser_state_t *state) {
  state->current_token++;
  assert(state->current_token <= darray_info(state->tokens)->count);
}

static void rollback_token_pointer(parser_state_t *state,
                                   uint32_t token_position) {
  assert(state->current_token <= darray_info(state->tokens)->count);
  state->current_token = token_position;
}

static token_t *get_token(parser_state_t *state) {
  return &state->tokens[state->current_token];
}

static bool is_comment(parser_state_t *state) {
  token_t *current_token = &state->tokens[state->current_token];
  return current_token->type == TOKEN_COMMENT;
}

static bool expect_and_consume(parser_state_t *state, e_token_type type) {
  token_t *token = get_token(state);
  if (token->type == type) {
    state->current_token += 1;
    return true;
  }
  return false;
}

static void add_to_symbol_table(parser_state_t *state, char *symbol,
                                e_token_type type, bool constant,
                                token_t *token, ast_node_t *node) {
  if (symbol_table_insert(state->current_scope, symbol, type, constant, node,
                          token->position.line) != SUCCESS) {
    // NOTE: Only one possible error for now
    // TODO: Use levenstein distance to look for typos?
    parse_error(
        state, token->position.line, token->position.column,
        "Undefined identifier '%s'.\n\nHint:  Perhaps you meant ...\n\n",
        symbol);
  }
}

static ast_node_t *make_node() { return imust_alloc(sizeof(ast_node_t)); }

static ast_node_t *parse_int_literal(parser_state_t *state) {
  token_t *token = get_token(state);
  if (token->type == TOKEN_INT_LITERAL) {
    ast_node_t *node = make_node();
    node->starting_token = token;
    node->line = token->position.line;
    node->column = token->position.column;
    node->type = ast_int_literal;
    node->literal.integer_value = atoi(token->value);
    node->total_tokens = 1;
    advance_token_pointer(state);
    return node;
  }
  return NULL;
}

static ast_node_t *parse_float_literal(parser_state_t *state) {
  token_t *token = get_token(state);
  if (token->type == TOKEN_FLOAT_LITERAL) {
    ast_node_t *node = make_node();
    node->starting_token = token;
    node->line = token->position.line;
    node->column = token->position.column;
    node->type = ast_float_literal;
    node->literal.float_value = atof(token->value);
    node->total_tokens = 1;
    advance_token_pointer(state);
    return node;
  }
  return NULL;
}

static ast_node_t *parse_str_literal(parser_state_t *state) {
  token_t *token = get_token(state);
  if (token->type == TOKEN_STR_LITERAL) {
    ast_node_t *node = make_node();
    node->starting_token = token;
    node->line = token->position.line;
    node->column = token->position.column;
    node->type = ast_str_literal;
    node->literal.string_value = token->value;
    node->total_tokens = 1;
    advance_token_pointer(state);
    return node;
  }
  return NULL;
}

static ast_node_t *parse_bool_literal(parser_state_t *state) {
  token_t *token = get_token(state);
  if (token->type == TOKEN_BOOL_LITERAL) {
    ast_node_t *node = make_node();
    node->starting_token = token;
    node->line = token->position.line;
    node->column = token->position.column;
    node->type = ast_bool_literal;
    node->literal.integer_value = streq(token->value, "true");
    node->total_tokens = 1;
    advance_token_pointer(state);
    return node;
  }
  return NULL;
}

static ast_node_t *parse_symbol(parser_state_t *state) {
  token_t *token = get_token(state);
  if (token->type == TOKEN_SYMBOL) {
    ast_node_t *node = make_node();
    node->starting_token = token;
    node->line = token->position.line;
    node->column = token->position.column;
    node->type = ast_symbol;
    node->literal.string_value = token->value;
    node->total_tokens = 1;
    advance_token_pointer(state);
    return node;
  }
  return NULL;
}

static ast_node_t *parse_fn_call(parser_state_t *state) {
  token_t *token = get_token(state);
  uint32_t starting_pos = state->current_token;
  if (token->type == TOKEN_SYMBOL) {
    ast_node_t *symbol = parse_symbol(state);
    if (expect_and_consume(state, TOKEN_PAREN_OPEN)) {
      da_nodes *exprs = darray_init(ast_node_t);
      do {
        ast_node_t *expr = parse_expr(state);
        if (expr)
          darray_append(exprs, *expr);
      } while (expect_and_consume(state, TOKEN_COMMA));
      if (expect_and_consume(state, TOKEN_PAREN_CLOSE)) {
        ast_node_t *node = make_node();
        node->starting_token = token;
        node->line = token->position.line;
        node->column = token->position.column;
        node->type = ast_fn_call;
        node->fn_call.symbol = symbol;
        node->fn_call.exprs = exprs;
        return node;
      }
    }
  }
  // We failed to parse an assignment rewind the parser state.
  rollback_token_pointer(state, starting_pos);
  return NULL;
}

static ast_node_t *parse_literal(parser_state_t *state) {
  token_t *token = get_token(state);
  switch (token->type) {
  case TOKEN_PAREN_OPEN: {
    uint32_t starting_pos = state->current_token;
    advance_token_pointer(state);
    ast_node_t *parenthasized_expr = parse_expr(state);
    if (parenthasized_expr) {
      token = get_token(state);
      if (token->type == TOKEN_PAREN_CLOSE) {
        advance_token_pointer(state);
        return parenthasized_expr;
      }
    }
    rollback_token_pointer(state, starting_pos);
    return NULL;
  }
  case TOKEN_INT_LITERAL:
    return parse_int_literal(state);
  case TOKEN_FLOAT_LITERAL:
    return parse_float_literal(state);
  case TOKEN_BOOL_LITERAL:
    return parse_bool_literal(state);
  case TOKEN_STR_LITERAL:
    return parse_str_literal(state);
  case TOKEN_SYMBOL: {
    // Could be a function call or an identifier
    ast_node_t *fn_call_node = parse_fn_call(state);
    if (fn_call_node)
      return fn_call_node;
    else
      return parse_symbol(state);
  }
  default: {
    return NULL;
  }
  }
}

static ast_node_t *parse_inner_term(parser_state_t *state,
                                    ast_node_t *longest) {
  if (longest) {
    token_t *op = get_token(state);
    if (op->type == TOKEN_MUL || op->type == TOKEN_QUO) {
      uint32_t starting_pos = state->current_token;
      advance_token_pointer(state);
      ast_node_t *literal = parse_literal(state);
      if (literal) {
        ast_node_t *node = make_node();
        node->starting_token = longest->starting_token;
        node->line = longest->starting_token->position.line;
        node->column = longest->starting_token->position.column;
        node->type = ast_term;
        node->expr.op = op->type;
        node->expr.left = longest;
        node->expr.right = literal;
        node->total_tokens = longest->total_tokens + 1 + literal->total_tokens;
        return node;
      } else {
        rollback_token_pointer(state, starting_pos);
      }
    }
  } else {
    ast_node_t *literal = parse_literal(state);
    if (literal)
      return literal;
  }
  return NULL;
}

static ast_node_t *parse_term(parser_state_t *state) {
  ast_node_t *longest = NULL;
  while (true) {
    ast_node_t *new_result = parse_inner_term(state, longest);
    if (new_result == NULL)
      break;
    if (longest != NULL && new_result->total_tokens <= longest->total_tokens)
      break;
    longest = new_result;
  }
  return longest;
}

static ast_node_t *parse_inner_expr(parser_state_t *state,
                                    ast_node_t *longest) {
  if (longest) {
    token_t *op = get_token(state);
    if (op->type == TOKEN_ADD || op->type == TOKEN_SUB ||
        op->type == TOKEN_EQL || op->type == TOKEN_NEQ) {
      uint32_t starting_pos = state->current_token;
      advance_token_pointer(state);
      ast_node_t *term = parse_term(state);
      if (term) {
        ast_node_t *node = make_node();
        node->starting_token = longest->starting_token;
        node->line = longest->starting_token->position.line;
        node->column = longest->starting_token->position.column;
        node->type = ast_expr;
        node->expr.op = op->type;
        node->expr.left = longest;
        node->expr.right = term;
        node->total_tokens = longest->total_tokens + 1 + term->total_tokens;
        return node;
      } else {
        rollback_token_pointer(state, starting_pos);
      }
    }
  } else {
    ast_node_t *term = parse_term(state);
    if (term)
      return term;
  }
  return NULL;
}

ast_node_t *parse_expr(parser_state_t *state) {
  ast_node_t *longest = NULL;
  while (true) {
    ast_node_t *new_result = parse_inner_expr(state, longest);
    if (new_result == NULL)
      break;
    if (longest != NULL && new_result->total_tokens <= longest->total_tokens)
      break;
    longest = new_result;
  }
  return longest;
}

static ast_node_t *must_parse_expr(parser_state_t *state) {
  ast_node_t *expr = parse_expr(state);
  if (expr == NULL) {
    token_t *err_token = get_token(state);
    parse_error(state, err_token->position.line, err_token->position.column,
                "Expected a valid expression.");
  }
  return expr;
}

static e_token_type parse_ika_type(parser_state_t *state) {
  token_t *token = get_token(state);
  if (token->type > _token_types_start && token->type < _token_types_end) {
    advance_token_pointer(state);
    return token->type;
  }
  return TOKEN_UNKNOWN;
}

// Declarations come in various flavors.  Mutable vs Unmutable.  With
// assignments, and without.
static ast_node_t *parse_decl(parser_state_t *state) {
  token_t *token = get_token(state);
  uint32_t starting_pos = state->current_token;
  b8 constant = FALSE;
  if (token->type == TOKEN_KEYWORD_LET) {
    constant = TRUE;
    advance_token_pointer(state);
    token = get_token(state);
  }
  ast_node_t *symbol = parse_symbol(state);
  if (symbol && expect_and_consume(state, TOKEN_COLON)) {
    e_token_type type = parse_ika_type(state);
    add_to_symbol_table(state, symbol->symbol.value, type, constant, token, 0);
    ast_node_t *node = make_node();
    node->type = ast_decl;
    node->starting_token = token;
    node->line = token->position.line;
    node->column = token->position.line;
    node->decl.symbol = symbol;
    node->decl.constant = constant;
    node->decl.type = type;
    token_t *next_token = get_token(state);
    if (type != TOKEN_UNKNOWN && next_token->type != TOKEN_ASSIGN &&
        !constant) { // Just a declaration
      node->decl.expr = NULL;
    } else if (next_token->type == TOKEN_ASSIGN) { // Declaration and assignment
      advance_token_pointer(state);
      node->decl.expr = must_parse_expr(state);
    } else if (type == TOKEN_UNKNOWN) {
      parse_error(state, next_token->position.line, next_token->position.column,
                  "Expected a type specifier or an expression assignment.");
    } else if (constant) {
      parse_error(state, next_token->position.line, next_token->position.column,
                  "Constants must be assigned a value at declaration time.");
    }
    return node;
  }
  rollback_token_pointer(state, starting_pos);
  return NULL;
}

static ast_node_t *parse_assignment(parser_state_t *state) {
  token_t *token = get_token(state);
  uint32_t starting_pos = state->current_token;
  if (token->type == TOKEN_SYMBOL) {
    ast_node_t *symbol = parse_symbol(state);
    if (symbol && expect_and_consume(state, TOKEN_ASSIGN)) {
      symbol_table_entry_t *var =
          symbol_table_lookup(state->current_scope, symbol->symbol.value);
      if (var) {
        ast_node_t *node = make_node();
        node->starting_token = token;
        node->line = token->position.line;
        node->column = token->position.column;
        node->type = ast_assignment;
        node->assignment.symbol = symbol;
        node->assignment.expr = must_parse_expr(state);
        return node;
      } else {
        // TODO:  Use levenstein distance to look for typos
        parse_error(
            state, symbol->line, symbol->column,
            "Undefined identifier '%s'.\n\nHint:  Perhaps you meant ...\n\n",
            symbol->symbol.value);
      }
    }
  }
  // We failed to parse an assignment rewind the parser state.
  rollback_token_pointer(state, starting_pos);
  return NULL;
}

static ast_node_t *parse_print_stmt(parser_state_t *state) {
  token_t *token = get_token(state);
  uint32_t starting_pos = state->current_token;
  if (token->type == TOKEN_KEYWORD_PRINT) {
    advance_token_pointer(state);
    ast_node_t *node = make_node();
    node->starting_token = token;
    node->line = token->position.line;
    node->column = token->position.column;
    node->type = ast_print_stmt;
    node->print_stmt.expr = must_parse_expr(state);
    return node;
  }
  // We failed to parse an assignment rewind the parser state.
  rollback_token_pointer(state, starting_pos);
  return NULL;
}

static ast_node_t *parse_block(parser_state_t *state) {
  token_t *token = get_token(state);
  uint32_t starting_pos = state->current_token;
  if (token->type == TOKEN_BRACE_OPEN) {
    // Make a new symbol table for the block/scope.  Store it in the parser
    // state and link it to the block/scope.  The current symbol table
    // must be restored afterwards.
    symbol_table_t *child_symbol_table =
        make_symbol_table(state->current_scope);
    state->current_scope = child_symbol_table;

    ast_node_t *node = make_node();
    node->starting_token = token;
    node->type = ast_block;
    node->line = token->position.line;
    node->column = token->position.line;
    node->block.nodes = darray_init(ast_node_t);
    node->block.symbol_table = child_symbol_table;
    advance_token_pointer(state); // Move past opening brace
    while (get_token(state)->type != TOKEN_BRACE_CLOSE) {
      ast_node_t *child_node = parse_node(state);
      if (child_node) {
        if (child_node->type == ast_return) {
          node->block.return_statement = child_node;
        } else if (!node->block.return_statement) {
          darray_append(node->block.nodes, *child_node);
        } else {
          parse_error(state, child_node->starting_token->position.line,
                      child_node->starting_token->position.column,
                      "Statement(s) after return.\n\nStatements after a return "
                      "have no effect\n\n");
        }
      }
    }
    advance_token_pointer(state); // Move past closing brace
    // Restore the scope to its previous state
    state->current_scope = state->current_scope->parent;
    return node;
  }
  rollback_token_pointer(state, starting_pos);
  return NULL;
}

static ast_node_t *parse_fn(parser_state_t *state) {
  symbol_table_t *function_scope = state->current_scope;
  symbol_table_t *params_symbol_table = NULL;
  token_t *token = get_token(state);
  uint32_t starting_pos = state->current_token;
  if (token->type == TOKEN_KEYWORD_FN) {
    advance_token_pointer(state);
    ast_node_t *symbol = parse_symbol(state);
    if (symbol) {
      da_nodes *decls = darray_init(ast_node_t);
      e_token_type return_type = TOKEN_VOID;
      if (expect_and_consume(state, TOKEN_PAREN_OPEN)) {
        // Function parameters are in their own scope
        params_symbol_table = make_symbol_table(state->current_scope);
        state->current_scope = params_symbol_table;
        do {
          ast_node_t *decl = parse_decl(state);
          if (decl)
            darray_append(decls, *decl);
        } while (expect_and_consume(state, TOKEN_COMMA));

        if (expect_and_consume(state, TOKEN_PAREN_CLOSE)) {
          if (expect_and_consume(state, TOKEN_COLON)) {
            token_t *next_token = get_token(state);
            if (next_token->type > _token_types_start &&
                next_token->type < _token_types_end) {
              return_type = next_token->type;
              advance_token_pointer(state);
            } else {
              // Error expected a type
            }
          } else {
            // Error missing closing parenthesis
          }
        }
      } else {
        token_t *err_token = get_token(state);
        parse_error(
            state, err_token->position.line, err_token->position.column,
            "Missing opening parenthesis for parameter list.\n\nFunctions "
            "require a parenthesized parameter list even if it's empty.\n\n");
        return NULL;
      }
      ast_node_t *block = parse_block(state);
      if (block) {
        // Restore scope to outer scope so function is defined in the proper
        // symbol table
        state->current_scope = function_scope;
        ast_node_t *node = make_node();
        node->starting_token = token;
        node->line = token->position.line;
        node->column = token->position.line;
        node->type = ast_fn;
        node->fn.symbol = symbol;
        node->fn.parameters = decls;
        node->fn.parameters_symbol_table = params_symbol_table;
        node->fn.block = block;
        node->fn.return_type = return_type;

        add_to_symbol_table(state, symbol->symbol.value, TOKEN_KEYWORD_FN, true,
                            token, node);
        return node;
      }
    }
  }
  state->current_scope = function_scope;
  rollback_token_pointer(state, starting_pos);
  return NULL;
}

static ast_node_t *parse_if_statement(parser_state_t *state) {
  token_t *token = get_token(state);
  uint32_t starting_pos = state->current_token;
  if (token->type == TOKEN_KEYWORD_IF) {
    advance_token_pointer(state);
    ast_node_t *expr = parse_expr(state);
    if (expr) {
      ast_node_t *if_block = parse_block(state);
      if (if_block) {
        ast_node_t *node = make_node();
        node->starting_token = token;
        node->line = token->position.line;
        node->column = token->position.line;
        node->type = ast_if_stmt;
        node->if_stmt.expr = expr;
        node->if_stmt.if_block = if_block;
        token = get_token(state);
        if (token->type == TOKEN_KEYWORD_ELSE) {
          advance_token_pointer(state);
          ast_node_t *else_block = parse_block(state);
          if (else_block) {
            node->if_stmt.else_block = else_block;
          } else {
            token_position_t pos = get_token(state)->position;
            parse_error(
                state, pos.line, pos.column,
                "Missing block for else clause.\n\nElse clauses require blocks "
                "of code to be executed if the else branch is choosen. \n\n "
                "Example:\n\n if (false) { x := 100} else { x:= 200 }\n");
          }
        }
        return node;
      }
    }
  }
  rollback_token_pointer(state, starting_pos);
  return NULL;
}

static ast_node_t *parse_return(parser_state_t *state) {
  token_t *token = get_token(state);
  uint32_t starting_pos = state->current_token;
  if (token->type == TOKEN_KEYWORD_RETURN) {
    advance_token_pointer(state);
    // Will be null in the case of a bare return
    ast_node_t *expr = parse_expr(state);

    ast_node_t *node = make_node();
    node->starting_token = token;
    node->line = token->position.line;
    node->column = token->position.line;
    node->type = ast_return;
    node->returns.expr = expr;
    return node;
  }
  rollback_token_pointer(state, starting_pos);
  return NULL;
}

ast_node_t *parse_node(parser_state_t *state) {
  ast_node_t *node;
  // Skip coments
  if (is_comment(state)) {
    advance_token_pointer(state);
    return NULL;
  }
  node = parse_print_stmt(state);
  if (node)
    return node;
  node = parse_assignment(state);
  if (node)
    return node;
  node = parse_if_statement(state);
  if (node)
    return node;
  node = parse_block(state);
  if (node)
    return node;
  node = parse_fn(state);
  if (node)
    return node;
  node = parse_fn_call(state);
  if (node)
    return node;
  node = parse_decl(state);
  if (node)
    return node;
  node = parse_return(state);
  if (node)
    return node;
  token_t *err_token = get_token(state);
  parse_error(state, err_token->position.line, err_token->position.column,
              "Unexpected token '%s'", err_token->value);
  // Skip the problematic token
  advance_token_pointer(state);
  return NULL;
}

ast_node_t *parser_parse(da_tokens *tokens, da_syntax_errors *errors) {
  parser_state_t parser_state =
      (parser_state_t){.current_token = 0, .tokens = tokens, .errors = errors};

  symbol_table_t *symbol_table = make_symbol_table(NULL);
  parser_state.current_scope = symbol_table;

  ast_node_t *root = make_node();
  root->starting_token = get_token(&parser_state);
  root->line = root->starting_token->position.line;
  root->column = root->starting_token->position.line;
  root->type = ast_block;
  root->block.symbol_table = symbol_table;
  da_nodes *child_nodes = darray_init(ast_node_t);
  while (parser_state.current_token < darray_info(parser_state.tokens)->count) {
    ast_node_t *node = parse_node(&parser_state);
    // Node parsing can return null in cases like comments, etc/
    if (node)
      darray_append(child_nodes, *node);
  }
  root->block.nodes = child_nodes;
  return root;
}
