#include <assert.h>

#include "../lib/assert.h"
#include "../lib/format.h"

#include "analyzer.h"
#include "ast.h"
#include "compiler.h"
#include "errors.h"
#include "symbol_table.h"
#include "types.h"

static void analyzer_error(analyzer_context_t ctx, u32 line, u32 column,
                           const char *fmt, ...) {
  syntax_error_t err = {.line = line, .column = column, .pass = TYPING};
  va_list args;
  va_start(args, fmt);
  err.message = format(fmt, args); // Leak
  darray_append(ctx.errors, err);
  va_end(args);
}

static e_ika_type determine_type_for_expression(analyzer_context_t ctx,
                                                ast_node_t *expression) {
  switch (expression->type) {
  case ast_int_literal:
    return ika_int;
  case ast_float_literal:
    return ika_float;
  case ast_bool_literal:
    return ika_bool;
  case ast_str_literal:
    return ika_str;
  case ast_symbol: {
    assert(ctx.parent->type == ast_block);
    symbol_table_entry_t *entry = symbol_table_lookup(
        ctx.parent->block.symbol_table, expression->symbol.value);
    if (entry) {
      return entry->type;
    } else {
      analyzer_error(
          ctx, expression->line, expression->column,
          "Undefined identifier '%s'.\n\nHint:  Perhaps you meant #todo\n\n",
          expression->symbol.value);
    }
    return ika_unknown;
  }
  case ast_term:
  case ast_expr: {
    e_ika_type ltype =
        determine_type_for_expression(ctx, expression->expr.left);
    e_ika_type rtype =
        determine_type_for_expression(ctx, expression->expr.right);
    if (ltype == rtype) {
      // Is a boolean expression: == != => <= > <
      if (expression->expr.op >= ika_eql && expression->expr.op <= ika_lte) {
        return ika_bool;
      } else {
        return ltype;
      }
    } else if ((ltype == ika_int && rtype == ika_float) ||
               (ltype == ika_float && rtype == ika_int)) {
      return ika_float;
    } else if (ltype == ika_unknown || rtype == ika_unknown) {
      // Punt till later, as it's most likely an error earlier in the analysis
      return ika_unknown;
    } else {
      analyzer_error(
          ctx, expression->expr.right->line, expression->expr.right->column,
          "Unsupported operation.\n\nThe %s operator is not supported between "
          "%s and %s.\n\n",
          ika_base_type_table[expression->expr.op].txt,
          ika_base_type_table[ltype].label, ika_base_type_table[rtype].label);
      return ika_unknown;
    }
  }
  case ast_fn_call: {
    symbol_table_entry_t *entry =
        symbol_table_lookup(ctx.parent->block.symbol_table,
                            expression->fn_call.symbol->symbol.value);
    ast_node_t *function = (ast_node_t *)entry->node_address;
    return function->fn.return_type;
  }
  default: {
    ASSERT_MSG((false), "Unexpected expression type of %d\n")
  }
  }
}

static void analyze_decl(analyzer_context_t ctx, ast_node_t *node) {
  if (node->decl.expr) {
    e_ika_type type = determine_type_for_expression(ctx, node->decl.expr);
    if (node->decl.type == ika_unknown) {
      node->decl.type = type;
    } else if (node->decl.type != type) {
      analyzer_error(
          ctx, node->decl.expr->line, node->decl.expr->column,
          "Type mismatch.\n\nCannot assign type %s to type %s, these "
          "types are not convertable.",
          ika_base_type_table[type].label,
          ika_base_type_table[node->decl.type].label);
    }
  }
}

static void analyze_assignment(analyzer_context_t ctx, ast_node_t *node) {
  symbol_table_entry_t *entry = symbol_table_lookup(
      ctx.parent->block.symbol_table, node->assignment.symbol->symbol.value);
  if (entry) {
    e_ika_type expr_type =
        determine_type_for_expression(ctx, node->assignment.expr);
    if (entry->type != expr_type) {
      analyzer_error(
          ctx, node->decl.expr->line, node->decl.expr->column,
          "Type mismatch.\n\nCannot assign type %s to type %s, these "
          "types are not convertable.",
          ika_base_type_table[expr_type].label,
          ika_base_type_table[entry->type].label);
    }
  } else {
    analyzer_error(ctx, node->line, node->column, "Undefined identifier");
  }
}

static void analyze_update_symbol_table(symbol_table_t *symbol_table,
                                        char *symbol, e_ika_type type) {
  symbol_table_entry_t *entry = symbol_table_lookup(symbol_table, symbol);
  if (entry) {
    entry->type = type;
  }
}

static void analyzer_resolve_function_return(analyzer_context_t ctx,
                                             ast_node_t *node) {

  // Simple case: the return statement is in the root function block
  if (node->fn.block->block.return_statement) {
    ctx.parent = node->fn.block; // Important: update the code to allow proper
                                 // symbol table lookup
    e_ika_type expected_return_type = node->fn.return_type;
    e_ika_type actual_return_type = determine_type_for_expression(
        ctx, node->fn.block->block.return_statement->returns.expr);
    if (expected_return_type != actual_return_type) {
      analyzer_error(
          ctx, node->fn.block->block.return_statement->returns.expr->line,
          node->fn.block->block.return_statement->returns.expr->column,
          "Unexpected type.\n\nThe function claims to return %s but "
          "this expression is of type %s.",
          ika_base_type_table[expected_return_type].label,
          ika_base_type_table[actual_return_type].label);
    }
  } else if (node->fn.return_type != ika_void) {
    ast_node_t *last_node =
        &node->fn.block->block
             .nodes[darray_len(node->fn.block->block.nodes) - 1];
    analyzer_error(ctx, last_node->line, last_node->column,
                   "Missing return statement");
  }
  // TODO: Eventually need to visit every child block recursively and check
  // for a return statement.  All paths must have a return.
}

static void analyze_fn_call(analyzer_context_t ctx, fn_call_t *fn_call) {
  symbol_table_entry_t *entry = symbol_table_lookup(
      ctx.parent->block.symbol_table, fn_call->symbol->symbol.value);
  ast_node_t *function = (ast_node_t *)entry->node_address;
  da_nodes *params = function->fn.parameters;
  da_nodes *exprs = fn_call->exprs;
  for (uint32_t i = 0; i < darray_len(params); i++) {
    ast_node_t *param = &params[i];
    ast_node_t *expr = &exprs[i];
    e_ika_type expr_type = determine_type_for_expression(ctx, expr);
    if (param->decl.type != expr_type) {
      char *param_name = param->decl.symbol->symbol.value;
      analyzer_error(ctx, expr->line, expr->column,
                     "Unexpected function argument type.\n\nParameter '%s' is "
                     "of type %s, but a %s was given.",
                     param_name, ika_base_type_table[param->decl.type].label,
                     ika_base_type_table[expr_type].label);
    }
  }
}

static void analyzer_resolve_types(analyzer_context_t ctx, ast_node_t *root) {
  assert(root->type == ast_block);
  symbol_table_t *current_symbol_table = root->block.symbol_table;
  ctx.parent = root;
  da_nodes *children = root->block.nodes;
  for (u64 i = 0; i < darray_len(children); i++) {
    ast_node_t *child = &children[i];
    switch (child->type) {
    case ast_decl:
      analyze_decl(ctx, child);
      analyze_update_symbol_table(current_symbol_table,
                                  child->decl.symbol->symbol.value,
                                  child->decl.type);
      break;
    case ast_assignment:
      analyze_assignment(ctx, child);
      break;
    case ast_block:
      analyzer_resolve_types(ctx, child);
      break;
    case ast_fn: {
      ast_node_t *orig_function = ctx.current_function;
      ctx.current_function = child;
      analyzer_resolve_types(ctx, child->fn.block);
      analyzer_resolve_function_return(ctx, child);
      ctx.current_function = orig_function;
      break;
    }
    case ast_if_statement: {
      e_ika_type type =
          determine_type_for_expression(ctx, child->if_statement.expr);
      if (type == ika_bool) {
        analyzer_resolve_types(ctx, child->if_statement.if_block);
        if (child->if_statement.else_block) {
          analyzer_resolve_types(ctx, child->if_statement.else_block);
        }
      } else {
        analyzer_error(ctx, child->if_statement.expr->line,
                       child->if_statement.expr->column,
                       "If expressions must be boolean.\n\nIf statement "
                       "expressions must evaluate to a boolean.\n\n");
      }
      break;
    }
    case ast_fn_call: {
      analyze_fn_call(ctx, &child->fn_call);
      break;
    }
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

void analyzer_analyze(compilation_unit_t *unit) {
  // Assumes the root node is a block
  assert(unit->root->type == ast_block);
  analyzer_context_t ctx = {
      .errors = unit->errors, .parent = NULL, .current_function = NULL};

  analyzer_resolve_types(ctx, unit->root);
}
