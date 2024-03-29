#include <assert.h>

#include "../lib/assert.h"
#include "../lib/format.h"

#include "ast.h"
#include "compiler.h"
#include "errors.h"
#include "symbol_table.h"
#include "tokens.h"
#include "typechecker.h"
#include "types.h"

static void tc_error(tc_context_t ctx, u32 line, u32 column, const char *fmt,
                     ...) {
  syntax_error_t err = {.line = line, .column = column, .pass = TYPING};
  va_list args;
  va_start(args, fmt);
  err.message = format(fmt, args); // Leak
  darray_append(ctx.errors, err);
  va_end(args);
}

static e_token_type determine_type_for_expression(tc_context_t ctx,
                                                  ast_node_t *expression) {
  switch (expression->type) {
  case ast_int_literal:
    return TOKEN_INT;
  case ast_float_literal:
    return TOKEN_FLOAT;
  case ast_bool_literal:
    return TOKEN_BOOL;
  case ast_str_literal:
    return TOKEN_STR;
  case ast_symbol: {
    assert(ctx.parent->type == ast_block);
    symbol_table_entry_t *entry = symbol_table_lookup(
        ctx.parent->block.symbol_table, expression->symbol.value);
    if (entry) {
      return entry->type;
    } else {
      tc_error(
          ctx, expression->line, expression->column,
          "Undefined identifier '%s'.\n\nHint:  Perhaps you meant #todo\n\n",
          expression->symbol.value);
    }
    return TOKEN_UNKNOWN;
  }
  case ast_term:
  case ast_expr: {
    e_token_type ltype =
        determine_type_for_expression(ctx, expression->expr.left);
    e_token_type rtype =
        determine_type_for_expression(ctx, expression->expr.right);
    if (ltype == rtype) {
      e_token_type ot = expression->expr.op;
      // Is a boolean expression: == != => <= > <
      if (ot == TOKEN_EQL || ot == TOKEN_NEQ || ot == TOKEN_GTE ||
          ot == TOKEN_LTE || ot == TOKEN_GT || ot == TOKEN_LT) {
        return TOKEN_BOOL;
      } else {
        return ltype;
      }
    } else if ((ltype == TOKEN_INT && rtype == TOKEN_FLOAT) ||
               (ltype == TOKEN_FLOAT && rtype == TOKEN_INT)) {
      return TOKEN_FLOAT;
    } else if (ltype == TOKEN_UNKNOWN || rtype == TOKEN_UNKNOWN) {
      // Punt till later, as it's most likely an error earlier in the analysis
      return TOKEN_UNKNOWN;
    } else {
      tc_error(
          ctx, expression->expr.right->line, expression->expr.right->column,
          "Unsupported operation.\n\nThe %s operator is not supported between "
          "%s and %s.\n\n",
          token_as_char[expression->expr.op], token_as_char[ltype],
          token_as_char[rtype]);
      return TOKEN_UNKNOWN;
    }
  }
  case ast_fn_call: {
    symbol_table_entry_t *entry =
        symbol_table_lookup(ctx.parent->block.symbol_table,
                            expression->fn_call.symbol->symbol.value);
    if (entry) {
      ast_node_t *function = (ast_node_t *)entry->node_address;
      return function->fn.return_type;
    } else {
      return TOKEN_UNKNOWN;
    }
  }
  default: {
    ASSERT_MSG((false), "Unexpected expression type of %d\n")
  }
  }
}

static void check_decl(tc_context_t ctx, ast_node_t *node) {
  if (node->decl.expr) {
    e_token_type type = determine_type_for_expression(ctx, node->decl.expr);
    if (node->decl.type == TOKEN_UNKNOWN) {
      node->decl.type = type;
    } else if (node->decl.type != type) {
      tc_error(ctx, node->decl.expr->line, node->decl.expr->column,
               "Type mismatch.\n\nCannot assign type %s to type %s, these "
               "types are not convertable.",
               token_as_char[type], token_as_char[node->decl.type]);
    }
  }
}

static void check_assignment(tc_context_t ctx, ast_node_t *node) {
  symbol_table_entry_t *entry = symbol_table_lookup(
      ctx.parent->block.symbol_table, node->assignment.symbol->symbol.value);
  if (entry && !entry->constant) {
    e_token_type expr_type =
        determine_type_for_expression(ctx, node->assignment.expr);
    if (entry->type != expr_type) {
      tc_error(ctx, node->decl.expr->line, node->decl.expr->column,
               "Type mismatch.\n\nCannot assign type %s to type %s, these "
               "types are not convertable.",
               token_as_char[expr_type], token_as_char[entry->type]);
    }
  } else if (entry && entry->constant) {
    tc_error(ctx, node->line, node->column,
             "Cannot assign a new value to a constant.\n\nIf you want to "
             "assign a new value to this symbol, then declare it mutable.");
  } else {
    tc_error(ctx, node->line, node->column, "Undefined identifier");
  }
}

static void check_print_stmt(tc_context_t ctx, ast_node_t *node) {
  e_token_type expr_type =
      determine_type_for_expression(ctx, node->print_stmt.expr);
  if (expr_type == TOKEN_UNKNOWN) {
    tc_error(ctx, node->print_stmt.expr->line, node->print_stmt.expr->column,
             "Cannot print expression as I cannot determine it's type.\n\n");
  }
}

static void update_symbol_table(symbol_table_t *symbol_table, char *symbol,
                                e_token_type type) {
  symbol_table_entry_t *entry = symbol_table_lookup(symbol_table, symbol);
  if (entry) {
    entry->type = type;
  }
}

static b8 tc_resolve_function_return(tc_context_t ctx, ast_node_t *node,
                                     e_token_type expected_return_type) {

  if (node->block.return_statement) {
    ctx.parent = node; // Important: update the code to allow proper
                       // symbol table lookup
    e_token_type actual_return_type = TOKEN_VOID;
    if (node->block.return_statement->returns.expr) {
      actual_return_type = determine_type_for_expression(
          ctx, node->block.return_statement->returns.expr);
    }
    if (expected_return_type != actual_return_type) {
      tc_error(ctx, node->block.return_statement->returns.expr->line,
               node->block.return_statement->returns.expr->column,
               "Unexpected type.\n\nThe function claims to return %s but "
               "this expression is of type %s.",
               token_as_char[expected_return_type],
               token_as_char[actual_return_type]);
    }
    return TRUE;
  } else {
    ast_node_t *children = node->block.nodes;
    for (u32 i = 0; i < darray_len(children); i++) {
      ast_node_t child = children[i];
      // Look for all the branching node types, and follow those paths checking
      // if returns statement are part of the branch.
      if (child.type == ast_if_stmt) {
        b8 if_block_return = tc_resolve_function_return(
            ctx, child.if_stmt.if_block, expected_return_type);
        if (child.if_stmt.else_block != NULL) {
          return if_block_return &&
                 tc_resolve_function_return(ctx, child.if_stmt.else_block,
                                            expected_return_type);
        }
      }
    }
    return expected_return_type == TOKEN_VOID ? TRUE : FALSE;
  }
}

static void check_fn_call(tc_context_t ctx, fn_call_t *fn_call) {
  symbol_table_entry_t *entry = symbol_table_lookup(
      ctx.parent->block.symbol_table, fn_call->symbol->symbol.value);
  if (entry) {
    ast_node_t *function = (ast_node_t *)entry->node_address;
    da_nodes *params = function->fn.parameters;
    da_nodes *exprs = fn_call->exprs;
    for (uint32_t i = 0; i < darray_len(params); i++) {
      ast_node_t *param = &params[i];
      ast_node_t *expr = &exprs[i];
      e_token_type expr_type = determine_type_for_expression(ctx, expr);
      if (param->decl.type != expr_type) {
        char *param_name = param->decl.symbol->symbol.value;
        tc_error(ctx, expr->line, expr->column,
                 "Unexpected function argument type.\n\nParameter '%s' is "
                 "of type %s, but a %s was given.",
                 param_name, token_as_char[param->decl.type],
                 token_as_char[expr_type]);
      }
    }
  }
}

static void tc_check_types(tc_context_t ctx, ast_node_t *root) {
  assert(root->type == ast_block);
  symbol_table_t *current_symbol_table = root->block.symbol_table;
  ctx.parent = root;
  da_nodes *children = root->block.nodes;
  for (u64 i = 0; i < darray_len(children); i++) {
    ast_node_t *child = &children[i];
    switch (child->type) {
    case ast_decl:
      check_decl(ctx, child);
      update_symbol_table(current_symbol_table,
                          child->decl.symbol->symbol.value, child->decl.type);
      break;
    case ast_assignment:
      check_assignment(ctx, child);
      break;
    case ast_block:
      tc_check_types(ctx, child);
      break;
    case ast_fn: {
      ast_node_t *orig_function = ctx.current_function;
      ctx.current_function = child;
      tc_check_types(ctx, child->fn.block);
      if (!tc_resolve_function_return(ctx, child->fn.block,
                                      child->fn.return_type)) {
        tc_error(ctx, child->line, child->column,
                 "A return statement is required on all control paths.");
      }
      ctx.current_function = orig_function;
      break;
    }
    case ast_if_stmt: {
      e_token_type type =
          determine_type_for_expression(ctx, child->if_stmt.expr);
      if (type == TOKEN_BOOL) {
        tc_check_types(ctx, child->if_stmt.if_block);
        if (child->if_stmt.else_block) {
          tc_check_types(ctx, child->if_stmt.else_block);
        }
      } else {
        tc_error(ctx, child->if_stmt.expr->line, child->if_stmt.expr->column,
                 "If expressions must be boolean.\n\nIf statement "
                 "expressions must evaluate to a boolean.\n\n");
      }
      break;
    }
    case ast_fn_call: {
      check_fn_call(ctx, &child->fn_call);
      break;
    }
    case ast_print_stmt:
      check_print_stmt(ctx, child);
      break;
    case ast_return:
    case ast_bool_literal:
    case ast_float_literal:
    case ast_str_literal:
    case ast_int_literal:
    case ast_expr:
    case ast_term:
    case ast_symbol:
      break;
    }
  }
}

void tc_check(compilation_unit_t *unit) {
  // Assumes the root node is a block
  assert(unit->root->type == ast_block);
  tc_context_t ctx = {
      .errors = unit->errors, .parent = NULL, .current_function = NULL};

  tc_check_types(ctx, unit->root);
}
