#include <assert.h>

#include "../lib/allocator.h"
#include "../lib/dynarray.h"
#include "../lib/format.h"

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
  err.message = format(state->allocator, fmt, args);
  dynarray_append(state->errors, &err);
}

ast_node_t *parse_node(parser_state_t *);
ast_node_t *parse_expr(parser_state_t *);

static void advance_token_pointer(parser_state_t *state) {
  state->current_token++;
  assert(state->current_token <= state->tokens->count);
}

static void rollback_token_pointer(parser_state_t *state,
                                   uint32_t token_position) {
  assert(token_position <= state->tokens->count);
  state->current_token = token_position;
}

static token_t *get_token(parser_state_t *state) {
  return (token_t *)dynarray_get(state->tokens, state->current_token);
}

static bool is_comment(parser_state_t *state) {
  token_t *current_token =
      (token_t *)dynarray_get(state->tokens, state->current_token);
  return current_token->type == ika_comment;
}

static bool expect_and_consume(parser_state_t *state, e_ika_type type) {
  token_t *token = get_token(state);
  if (token->type == type) {
    state->current_token += 1;
    return true;
  }
  return false;
}

static void add_to_symbol_table(parser_state_t *state, char *symbol,
                                e_ika_type type, bool constant, token_t *token,
                                ast_node_t *node) {
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

static ast_node_t *make_node(allocator_t allocator) {
  ast_node_t *node = allocator_alloc_or_exit(allocator, sizeof(ast_node_t));
  return node;
}

static ast_node_t *parse_int_literal(parser_state_t *state) {
  token_t *token = get_token(state);
  if (token->type == ika_int_literal) {
    ast_node_t *node = make_node(state->allocator);
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
  if (token->type == ika_float_literal) {
    ast_node_t *node = make_node(state->allocator);
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
  if (token->type == ika_str_literal) {
    ast_node_t *node = make_node(state->allocator);
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
  if (token->type == ika_bool_literal) {
    ast_node_t *node = make_node(state->allocator);
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
  if (token->type == ika_symbol) {
    ast_node_t *node = make_node(state->allocator);
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
  if (token->type == ika_symbol) {
    ast_node_t *symbol = parse_symbol(state);
    if (expect_and_consume(state, ika_paren_open)) {
      dynarray *exprs = dynarray_init(state->allocator, sizeof(ast_node_t));
      do {
        ast_node_t *expr = parse_expr(state);
        if (expr)
          dynarray_append(exprs, expr);
      } while (expect_and_consume(state, ika_comma));
      if (expect_and_consume(state, ika_paren_close)) {
        ast_node_t *node = make_node(state->allocator);
        node->starting_token = token;
        node->line = token->position.line;
        node->column = token->position.column;
        node->type = ast_fn_call;
        node->fn_call.symbol = symbol;
        node->fn_call.exprs = *exprs;
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
  case ika_paren_open: {
    uint32_t starting_pos = state->current_token;
    advance_token_pointer(state);
    ast_node_t *parenthasized_expr = parse_expr(state);
    if (parenthasized_expr) {
      token = get_token(state);
      if (token->type == ika_paren_close) {
        advance_token_pointer(state);
        return parenthasized_expr;
      }
    }
    rollback_token_pointer(state, starting_pos);
    return NULL;
  }
  case ika_int_literal:
    return parse_int_literal(state);
  case ika_float_literal:
    return parse_float_literal(state);
  case ika_bool_literal:
    return parse_bool_literal(state);
  case ika_str_literal:
    return parse_str_literal(state);
  case ika_symbol: {
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
    if (op->type == ika_mul || op->type == ika_quo) {
      uint32_t starting_pos = state->current_token;
      advance_token_pointer(state);
      ast_node_t *literal = parse_literal(state);
      if (literal) {
        ast_node_t *node = make_node(state->allocator);
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
    if (op->type == ika_add || op->type == ika_sub || op->type == ika_eql ||
        op->type == ika_neq) {
      uint32_t starting_pos = state->current_token;
      advance_token_pointer(state);
      ast_node_t *term = parse_term(state);
      if (term) {
        ast_node_t *node = make_node(state->allocator);
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

static e_ika_type parse_assignment_type(parser_state_t *state) {
  uint32_t starting_pos = state->current_token;
  if (expect_and_consume(state, ika_colon)) {
    token_t *token = get_token(state);
    e_ika_type type;
    if (token->type > __ika_types_start && token->type < __ika_types_end) {
      type = token->type;
      advance_token_pointer(state);
      if (expect_and_consume(state, ika_assign)) {
        return type;
      }
    }
  }
  rollback_token_pointer(state, starting_pos);
  return ika_unknown;
}

static ast_node_t *parse_assignment(parser_state_t *state) {
  token_t *token = get_token(state);
  uint32_t starting_pos = state->current_token;
  bool is_definition = false;
  bool constant = false;
  if (token->type == ika_keyword_let) {
    constant = true;
    advance_token_pointer(state);
    token = get_token(state);
  }
  if (token->type == ika_symbol) {
    ast_node_t *symbol = parse_symbol(state);
    e_ika_type type = ika_unknown;
    if (symbol) {
      // Assignment to a previously defined var aka x = 10
      if (expect_and_consume(state, ika_assign)) {
        symbol_table_entry_t *var =
            symbol_table_lookup(state->current_scope, symbol->symbol.value);
        if (var) {
          is_definition = false;
          type = var->type;
        } else {
          // Trying to assign to an undefined symbol is an error
          // TODO:  Use levenstein distance to look for typos
          parse_error(
              state, symbol->line, symbol->column,
              "Undefined identifier '%s'.\n\nHint:  Perhaps you meant ...\n\n",
              symbol->symbol.value);
          type = ika_unknown;
        }
        // Untyped assign aka x := 10
      } else if (expect_and_consume(state, ika_untyped_assign)) {
        is_definition = true;
        type = ika_untyped_assign;
        // A typed assign aka x : int = 10
      } else {
        is_definition = true;
        type = parse_assignment_type(state);
      }
      if (type != ika_unknown) {
        ast_node_t *expr = parse_expr(state);
        if (expr) {
          if (is_definition)
            add_to_symbol_table(state, symbol->symbol.value, type, constant,
                                token, 0);
          ast_node_t *node = make_node(state->allocator);
          node->starting_token = dynarray_get(state->tokens, starting_pos);
          node->line = node->starting_token->position.line;
          node->column = node->starting_token->position.column;
          node->type = ast_assignment;
          node->assignment.symbol = symbol;
          node->assignment.type = type;
          node->assignment.expr = expr;
          node->assignment.constant = constant;
          return node;
        }
      }
    }
  }
  // We failed to parse an assignment rewind the parser state.
  rollback_token_pointer(state, starting_pos);
  return NULL;
}

static e_ika_type parse_ika_type_decl(parser_state_t *state) {
  uint32_t starting_pos = state->current_token;
  if (expect_and_consume(state, ika_colon)) {
    token_t *token = get_token(state);
    e_ika_type type;
    if (token->type > __ika_types_start && token->type < __ika_types_end) {
      type = token->type;
      advance_token_pointer(state);
      return type;
    }
  }
  rollback_token_pointer(state, starting_pos);
  return ika_unknown;
}

// TODO: Use this in assignment and remove untyped assign.
static ast_node_t *parse_decl(parser_state_t *state) {
  token_t *token = get_token(state);
  uint32_t starting_pos = state->current_token;
  if (token->type == ika_symbol) {
    ast_node_t *symbol = parse_symbol(state);
    if (symbol) {
      e_ika_type type = parse_ika_type_decl(state);
      if (type != ika_unknown) {
        add_to_symbol_table(state, symbol->symbol.value, type, false, token, 0);
        ast_node_t *node = make_node(state->allocator);
        node->type = ast_decl;
        node->starting_token = token;
        node->line = token->position.line;
        node->column = token->position.line;
        node->decl.symbol = symbol;
        node->decl.type = type;
        return node;
      }
    }
  }
  rollback_token_pointer(state, starting_pos);
  return NULL;
}

// TODO: Maybe rename block to scope
static ast_node_t *parse_block(parser_state_t *state) {
  token_t *token = get_token(state);
  uint32_t starting_pos = state->current_token;
  if (token->type == ika_brace_open) {
    // Make a new symbol table for the block/scope.  Store it in the parser
    // state and link it to the block/scope.  The current symbol table
    // must be restored afterwards.
    symbol_table_t *child_symbol_table =
        make_symbol_table(state->allocator, state->current_scope);
    state->current_scope = child_symbol_table;

    ast_node_t *node = make_node(state->allocator);
    node->starting_token = token;
    node->type = ast_block;
    node->line = token->position.line;
    node->column = token->position.line;
    node->block.nodes = *dynarray_init(state->allocator, sizeof(ast_node_t));
    node->block.symbol_table = child_symbol_table;
    advance_token_pointer(state); // Move past opening brace
    while (get_token(state)->type != ika_brace_close) {
      ast_node_t *child_node = parse_node(state);
      if (child_node->type == ast_return) {
        node->block.return_statement = child_node;
      } else if (!node->block.return_statement) {
        dynarray_append(&node->block.nodes, child_node);
      } else {
        parse_error(state, child_node->starting_token->position.line,
                    child_node->starting_token->position.column,
                    "Statement(s) after return.\n\nStatements after a return "
                    "have no effect\n\n");
      }
    }
    advance_token_pointer(state); // Move past closing brace
    // Restore the symbol table to the previous scopes table
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
  if (token->type == ika_keyword_fn) {
    advance_token_pointer(state);
    ast_node_t *symbol = parse_symbol(state);
    if (symbol) {
      dynarray *decls = dynarray_init(state->allocator, sizeof(ast_node_t));
      e_ika_type return_type = ika_void;
      if (expect_and_consume(state, ika_paren_open)) {
        // Function parameters are in their own scope
        //
        // TODO: Think about unifying scope/symbol tables
        params_symbol_table =
            make_symbol_table(state->allocator, state->current_scope);
        state->current_scope = params_symbol_table;
        do {
          ast_node_t *decl = parse_decl(state);
          if (decl)
            dynarray_append(decls, decl);
        } while (expect_and_consume(state, ika_comma));

        if (expect_and_consume(state, ika_paren_close)) {
          if (expect_and_consume(state, ika_colon)) {
            token_t *next_token = get_token(state);
            if (next_token->type > __ika_types_start &&
                next_token->type < __ika_types_end) {
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
        ast_node_t *node = make_node(state->allocator);
        node->starting_token = token;
        node->line = token->position.line;
        node->column = token->position.line;
        node->type = ast_fn;
        node->fn.symbol = symbol;
        node->fn.parameters = *decls;
        node->fn.parameters_symbol_table = params_symbol_table;
        node->fn.block = block;
        node->fn.return_type = return_type;

        add_to_symbol_table(state, symbol->symbol.value, ika_keyword_fn, true,
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
  if (token->type == ika_keyword_if) {
    advance_token_pointer(state);
    ast_node_t *expr = parse_expr(state);
    if (expr) {
      ast_node_t *if_block = parse_block(state);
      if (if_block) {
        ast_node_t *node = make_node(state->allocator);
        node->starting_token = token;
        node->line = token->position.line;
        node->column = token->position.line;
        node->type = ast_if_statement;
        node->if_statement.expr = expr;
        node->if_statement.if_block = if_block;
        token = get_token(state);
        if (token->type == ika_keyword_else) {
          advance_token_pointer(state);
          ast_node_t *else_block = parse_block(state);
          if (else_block) {
            node->if_statement.else_block = else_block;
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
  if (token->type == ika_keyword_return) {
    advance_token_pointer(state);
    // Will be null in the case of a bare return
    ast_node_t *expr = parse_expr(state);

    ast_node_t *node = make_node(state->allocator);
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

ast_node_t *parser_parse(allocator_t allocator, dynarray *tokens,
                         dynarray *errors) {
  parser_state_t parser_state = (parser_state_t){.current_token = 0,
                                                 .tokens = tokens,
                                                 .allocator = allocator,
                                                 .errors = errors};

  symbol_table_t *symbol_table = make_symbol_table(allocator, NULL);
  parser_state.current_scope = symbol_table;

  ast_node_t *root = make_node(allocator);
  root->starting_token = get_token(&parser_state);
  root->line = root->starting_token->position.line;
  root->column = root->starting_token->position.line;
  root->type = ast_block;
  root->block.symbol_table = symbol_table;
  root->block.nodes = *dynarray_init(allocator, sizeof(ast_node_t));
  while (parser_state.current_token < parser_state.tokens->count) {
    ast_node_t *node = parse_node(&parser_state);
    // Node parsing can return null in cases like comments, etc/
    if (node)
      dynarray_append(&root->block.nodes, node);
  }

  return root;
}
