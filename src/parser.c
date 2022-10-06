#include <assert.h>

#include "../lib/allocator.h"
#include "../lib/dynarray.h"

#include "parser.h"
#include "tokenize.h"
#include "types.h"

// TODO: Handle function declarations
// TODO: Handle return statement
// TODO: First pass at error handling

ast_node_t *parse_node(parser_state_t *);
ast_node_t *parse_expr(parser_state_t *);

void advance_token_pointer(parser_state_t *state) {
  state->current_token++;
  assert(state->current_token <= state->tokens->count);
}

void rollback_token_pointer(parser_state_t *state, uint32_t token_position) {
  assert(token_position <= state->tokens->count);
  state->current_token = token_position;
}

token_t *get_token(parser_state_t *state) {
  return (token_t *)dynarray_get(state->tokens, state->current_token);
}

bool is_comment(parser_state_t *state) {
  token_t *current_token =
      (token_t *)dynarray_get(state->tokens, state->current_token);
  return current_token->type == ika_comment;
}

bool expect_and_consume(parser_state_t *state, e_ika_type type) {
  token_t *token = get_token(state);
  if (token->type == type) {
    state->current_token += 1;
    return true;
  }
  return false;
}

ast_node_t *make_node(allocator_t allocator) {
  ast_node_t *node = allocator_alloc_or_exit(allocator, sizeof(ast_node_t));
  return node;
}

ast_node_t *parse_int_literal(parser_state_t *state) {
  token_t *token = get_token(state);
  if (token->type == ika_int_literal || token->type == ika_num_literal) {
    ast_node_t *node = make_node(state->allocator);
    node->starting_token = token;
    node->type = ast_int_literal;
    node->literal.integer_value = atoi(token->value.ptr);
    node->total_tokens = 1;
    advance_token_pointer(state);
    return node;
  }
  return NULL;
}

ast_node_t *parse_float_literal(parser_state_t *state) {
  token_t *token = get_token(state);
  if (token->type == ika_float_literal) {
    ast_node_t *node = make_node(state->allocator);
    node->starting_token = token;
    node->type = ast_float_literal;
    node->literal.float_value = atof(token->value.ptr);
    node->total_tokens = 1;
    advance_token_pointer(state);
    return node;
  }
  return NULL;
}

ast_node_t *parse_str_literal(parser_state_t *state) {
  token_t *token = get_token(state);
  if (token->type == ika_str_literal) {
    ast_node_t *node = make_node(state->allocator);
    node->starting_token = token;
    node->type = ast_str_literal;
    node->literal.string_value = token->value;
    node->total_tokens = 1;
    advance_token_pointer(state);
    return node;
  }
  return NULL;
}

ast_node_t *parse_bool_literal(parser_state_t *state) {
  token_t *token = get_token(state);
  if (token->type == ika_bool_literal) {
    ast_node_t *node = make_node(state->allocator);
    node->starting_token = token;
    node->type = ast_bool_literal;
    node->literal.integer_value = str_eq(token->value, cstr("true"));
    node->total_tokens = 1;
    advance_token_pointer(state);
    return node;
  }
  return NULL;
}

ast_node_t *parse_symbol(parser_state_t *state) {
  token_t *token = get_token(state);
  if (token->type == ika_identifier) {
    ast_node_t *node = make_node(state->allocator);
    node->starting_token = token;
    node->type = ast_symbol;
    node->literal.string_value = token->value;
    node->total_tokens = 1;
    advance_token_pointer(state);
    return node;
  }
  return NULL;
}

ast_node_t *parse_literal(parser_state_t *state) {
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
  case ika_num_literal:
    return parse_int_literal(state);
  case ika_float_literal:
    return parse_float_literal(state);
  case ika_bool_literal:
    return parse_bool_literal(state);
  case ika_str_literal:
    return parse_str_literal(state);
  default: {
    return NULL;
  }
  }
}

ast_node_t *parse_inner_term(parser_state_t *state, ast_node_t *longest) {
  if (longest) {
    token_t *op = get_token(state);
    if (op->type == ika_mul || op->type == ika_quo) {
      uint32_t starting_pos = state->current_token;
      advance_token_pointer(state);
      ast_node_t *literal = parse_literal(state);
      if (literal) {
        ast_node_t *node = make_node(state->allocator);
        node->starting_token = longest->starting_token;
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

ast_node_t *parse_term(parser_state_t *state) {
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

ast_node_t *parse_inner_expr(parser_state_t *state, ast_node_t *longest) {
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

e_ika_type parse_assignment_type(parser_state_t *state) {
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

ast_node_t *parse_assignment(parser_state_t *state) {
  token_t *token = get_token(state);
  uint32_t starting_pos = state->current_token;
  bool constant = false;
  if (token->type == ika_keyword_let) {
    constant = true;
    advance_token_pointer(state);
    token = get_token(state);
  }
  if (token->type == ika_identifier) {
    ast_node_t *symbol = parse_symbol(state);
    e_ika_type type = ika_unknown;
    if (symbol) {
      if (expect_and_consume(state, ika_untyped_assign)) {
        type = ika_untyped_assign;
      } else {
        type = parse_assignment_type(state);
      }
      if (type != ika_unknown) {
        ast_node_t *expr = parse_expr(state);
        if (expr) {
          ast_node_t *node = make_node(state->allocator);
          node->starting_token = dynarray_get(state->tokens, starting_pos);
          node->type = ast_assignment;
          node->assignment.identifier = symbol;
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

e_ika_type parse_ika_type_decl(parser_state_t *state) {
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
ast_node_t *parse_decl(parser_state_t *state) {
  token_t *token = get_token(state);
  uint32_t starting_pos = state->current_token;
  if (token->type == ika_identifier) {
    ast_node_t *symbol = parse_symbol(state);
    if (symbol) {
      e_ika_type type = parse_ika_type_decl(state);
      if (type != ika_unknown) {
        ast_node_t *node = make_node(state->allocator);
        node->type = ast_decl;
        node->decl.identifier = symbol;
        node->decl.type = type;
        return node;
      }
    }
  }
  rollback_token_pointer(state, starting_pos);
  return NULL;
}

ast_node_t *parse_block(parser_state_t *state) {
  token_t *token = get_token(state);
  uint32_t starting_pos = state->current_token;
  if (token->type == ika_brace_open) {
    ast_node_t *node = make_node(state->allocator);
    node->starting_token = token;
    node->type = ast_block;
    node->block.nodes = *dynarray_init(state->allocator, sizeof(ast_node_t));
    advance_token_pointer(state); // Move past opening brace
    while (get_token(state)->type != ika_brace_close) {
      ast_node_t *child_node = parse_node(state);
      dynarray_append(&node->block.nodes, child_node);
    }
    advance_token_pointer(state); // Move past closing brace
    return node;
  }
  rollback_token_pointer(state, starting_pos);
  return NULL;
}

ast_node_t *parse_fn(parser_state_t *state) {
  token_t *token = get_token(state);
  uint32_t starting_pos = state->current_token;
  if (token->type == ika_keyword_fn) {
    advance_token_pointer(state);
    ast_node_t *symbol = parse_symbol(state);
    printf("parse_fn: %p\n", symbol);
    if (symbol) {
      dynarray *decls = dynarray_init(state->allocator, sizeof(ast_node_t));
      dynarray *returns = dynarray_init(state->allocator, sizeof(ast_node_t));
      if (expect_and_consume(state, ika_paren_open)) {
        // TODO: Use do {} while
        ast_node_t *decl = parse_decl(state);
        if (decl) {
          dynarray_append(decls, decl);
          while (expect_and_consume(state, ika_comma)) {
            decl = parse_decl(state);
            if (decl)
              dynarray_append(decls, decl);
          }
        }
      }
      if (expect_and_consume(state, ika_paren_close)) {
        if (expect_and_consume(state, ika_colon)) {
          token_t *next_token = get_token(state);
          if (next_token->type > __ika_types_start &&
              next_token->type < __ika_types_end) {
            dynarray_append(returns, &next_token->type);
            advance_token_pointer(state);
          } else if (next_token->type == ika_paren_open) {
            advance_token_pointer(state);
            do {
              next_token = get_token(state);
              if (next_token->type > __ika_types_start &&
                  next_token->type < __ika_types_end)
                dynarray_append(returns, &next_token->type);
              advance_token_pointer(state);
            } while (expect_and_consume(state, ika_comma));
            if (!expect_and_consume(state, ika_paren_close)) {
              next_token = get_token(state);
              printf("Syntax error, expected a closing parenthesis, got %d\n",
                     next_token->type);
              exit(-1);
            }
          } else {
            // Error handling
          }
        }
        ast_node_t *block = parse_block(state);
        if (block) {
          ast_node_t *node = make_node(state->allocator);
          node->type = ast_fn;
          node->fn.identifer = symbol;
          node->fn.parameters = *decls;
          node->fn.block = block;
          node->fn.return_types = *returns;
          return node;
        }
      }
    }
  }
  rollback_token_pointer(state, starting_pos);
  return NULL;
}

ast_node_t *parse_if_statement(parser_state_t *state) {
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
            printf("Expecting else block at %d, %d.", token->position.line,
                   token->position.column);
          }
        }
        return node;
      }
    }
  }
  rollback_token_pointer(state, starting_pos);
  return NULL;
}

ast_node_t *parse_return(parser_state_t *state) {
  token_t *token = get_token(state);
  uint32_t starting_pos = state->current_token;
  if (token->type == ika_keyword_return) {
    advance_token_pointer(state);
    token_t *next_token = get_token(state);
    dynarray *returns = dynarray_init(state->allocator, sizeof(ast_node_t));
    if (next_token->type == ika_paren_open) {
      advance_token_pointer(state);
      do {
        ast_node_t *expr = parse_expr(state);
        if (expr)
          dynarray_append(returns, expr);
      } while (expect_and_consume(state, ika_comma));
      if (!expect_and_consume(state, ika_paren_close)) {
        next_token = get_token(state);
        printf("Syntax error, expected a closing parenthesis, got %d\n",
               next_token->type);
        exit(-1);
      }
    } else {
      ast_node_t *expr = parse_expr(state);
      if (expr)
        dynarray_append(returns, expr);
    }
    ast_node_t *node = make_node(state->allocator);
    node->starting_token = token;
    node->type = ast_return;
    node->returns.exprs = *returns;
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
  node = parse_expr(state);
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
  node = parse_decl(state);
  if (node)
    return node;
  node = parse_return(state);
  if (node)
    return node;
  token_t *errorneous_token = get_token(state);
  printf("Unexpected token '%.*s' on line %i\n", errorneous_token->value.length,
         errorneous_token->value.ptr, errorneous_token->position.line);
  // Skip the problematic token
  advance_token_pointer(state);
  return NULL;
}

ast_node_t *parser_parse(compilation_unit_t *unit) {
  parser_state_t parser_state = (parser_state_t){
      .current_token = 0, .tokens = unit->tokens, .allocator = unit->allocator};
  ast_node_t *root = make_node(unit->allocator);
  root->starting_token = get_token(&parser_state);
  root->type = ast_block;
  root->block.nodes = *dynarray_init(unit->allocator, sizeof(ast_node_t));
  while (parser_state.current_token < parser_state.tokens->count) {
    ast_node_t *node = parse_node(&parser_state);
    // Node parsing can return null in cases like comments, etc/
    if (node)
      dynarray_append(&root->block.nodes, node);
  }
  return root;
}
