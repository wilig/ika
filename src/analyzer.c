#include <assert.h>

#include "analyzer.h"
#include "ast.h"
#include "compiler.h"
#include "errors.h"
#include "symbol_table.h"
#include "types.h"

e_ika_type determine_type_for_expression(analyzer_context_t ctx,
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
    symbol_table_entry_t *entry =
        symbol_table_lookup(ctx.symbol_table, expression->symbol.value);
    if (entry) {
      return entry->type;
    } else {
      // TODO: Possibly use levenstein distance to find likely typos
      printf("failed to lookup symbol, anding error to error list\n");
      syntax_error_t err = {.column = expression->column,
                            .line = expression->line,
                            .pass = typing_pass,
                            .message = cstr("Undefined identifier")};
      dynarray_append(ctx.errors, &err);
    }
    return ika_unknown;
  }
  case ast_term:
  case ast_expr: {
    e_ika_type ltype =
        determine_type_for_expression(ctx, expression->expr.left);
    e_ika_type rtype =
        determine_type_for_expression(ctx, expression->expr.right);
    if (ltype == rtype) { // Simple case
      return ltype;
    } else if (ltype == ika_int && rtype == ika_float ||
               ltype == ika_float && rtype == ika_int) {
      return ika_float;
    } else if (ltype == ika_unknown || rtype == ika_unknown) {
      // Punt till later, as it's most likely an error earlier in the analysis
      return ika_unknown;
    } else {
      // TODO: Need a way to do string interpolation on these messages
      syntax_error_t err = {.column = expression->column,
                            .line = expression->line,
                            .pass = typing_pass,
                            .message = cstr("Unsupported operation"),
                            .hint = cstr("The operation not support for given "
                                         "types (#needs interpolation)")};
      dynarray_append(ctx.errors, &err);
      return ika_unknown;
    }
  }
  default: {
    assert(false);
  }
  }
}

void analyze_assignment(analyzer_context_t ctx, ast_node_t *node) {
  e_ika_type type = determine_type_for_expression(ctx, node->assignment.expr);
  if (node->assignment.type == ika_untyped_assign) {
    node->assignment.type = type;
  } else if (node->assignment.type != type) {
    syntax_error_t err = {
        .column = node->column,
        .line = node->line,
        .pass = typing_pass,
        .message = cstr("Type mismatch"),
        .hint = cstr("The types don't match (#needs interpolation)")};
    dynarray_append(ctx.errors, &err);
  }
}

void analyze_update_symbol_table(symbol_table_t *symbol_table, str identifer,
                                 e_ika_type type) {
  symbol_table_entry_t *entry = symbol_table_lookup(symbol_table, identifer);
  if (entry) {
    entry->type = type;
  }
}

// DONE: Do whole tree, not just the root node
// DONE: Error handling!
// DONE: Symbol lookup
// TODO: Function analysis
// TODO: If statement analysis
// TODO: High/medium level IR perhaps?
void analyzer_resolve_types(analyzer_context_t ctx, ast_node_t *root) {
  assert(root->type == ast_block);
  dynarray children = root->block.nodes;
  for (int i = 0; i < children.count; i++) {
    ast_node_t *child = dynarray_get(&children, i);
    switch (child->type) {
    case ast_assignment:
      analyze_assignment(ctx, child);
      analyze_update_symbol_table(ctx.symbol_table,
                                  child->assignment.identifier->symbol.value,
                                  child->assignment.type);
      break;

    case ast_block:
      ctx.symbol_table = child->block.symbol_table;
      analyzer_resolve_types(ctx, child);
      break;

    default:
      break;
    }
  }
}

void analyzer_analyze(compilation_unit_t *unit) {
  // Assumes the root node is a block
  assert(unit->root->type == ast_block);
  analyzer_context_t ctx = {.errors = unit->errors,
                            .symbol_table = unit->root->block.symbol_table};

  analyzer_resolve_types(ctx, unit->root);
  printf("Errors count is %li\n", ctx.errors->count);
}
