#include <assert.h>

#include "../lib/format.h"

#include "analyzer.h"
#include "ast.h"
#include "compiler.h"
#include "errors.h"
#include "symbol_table.h"
#include "types.h"

static void analyzer_report_syntax_error(analyzer_context_t ctx,
                                         ast_node_t *node, str message,
                                         str hint) {
  syntax_error_t err = {.column = node->column,
                        .line = node->line,
                        .pass = typing_pass,
                        .message = message,
                        .hint = hint};
  dynarray_append(ctx.errors, &err);
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
      // TODO: Possibly use levenstein distance to find likely typos
      analyzer_report_syntax_error(
          ctx, expression, cstr("Undefined identifier"),
          format(ctx.allocator, "'%s' is undefined within this scope",
                 str_to_cstr(ctx.allocator, expression->symbol.value)));
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
      analyzer_report_syntax_error(
          ctx, expression->expr.right, cstr("Unsupported operation"),
          format(ctx.allocator,
                 "The %s operator is not supported between %s and %s.",
                 ika_base_type_table[expression->expr.op].txt,
                 ika_base_type_table[ltype].label,
                 ika_base_type_table[rtype].label));
      return ika_unknown;
    }
  }
  default: {
    assert(false);
  }
  }
}

static void analyze_assignment(analyzer_context_t ctx, ast_node_t *node) {
  e_ika_type type = determine_type_for_expression(ctx, node->assignment.expr);
  if (node->assignment.type == ika_untyped_assign) {
    node->assignment.type = type;
  } else if (node->assignment.type != type) {
    analyzer_report_syntax_error(
        ctx, node->assignment.expr, cstr("Type mismatch"),
        format(ctx.allocator,
               "Cannot assign type %s to type %s, these types are not "
               "convertable.",
               ika_base_type_table[type].label,
               ika_base_type_table[node->assignment.type].label));
  }
}

static void analyze_update_symbol_table(symbol_table_t *symbol_table,
                                        str identifer, e_ika_type type) {
  symbol_table_entry_t *entry = symbol_table_lookup(symbol_table, identifer);
  if (entry) {
    entry->type = type;
  }
}

static void analyzer_type_check_function_return(analyzer_context_t ctx,
                                                ast_node_t *return_node,
                                                dynarray expected_return_types,
                                                dynarray actual_return_exprs) {
  if (expected_return_types.count == actual_return_exprs.count) {
    for (uint32_t rt = 0; rt < expected_return_types.count; rt++) {
      e_ika_type *expected_return_type =
          dynarray_get(&expected_return_types, rt);
      ast_node_t *return_expr = dynarray_get(&actual_return_exprs, rt);
      e_ika_type actual_return_type =
          determine_type_for_expression(ctx, return_expr);
      if (*expected_return_type != actual_return_type) {
        analyzer_report_syntax_error(
            ctx, return_expr, cstr("Unexpected type"),
            format(ctx.allocator,
                   "The function claims to return %s but this expression "
                   "is of type %s.",
                   ika_base_type_table[*expected_return_type].label,
                   ika_base_type_table[actual_return_type].label));
      }
    }
  } else {
    analyzer_report_syntax_error(
        ctx, return_node, cstr("Return argument mismatch"),
        format(ctx.allocator,
               "The function claims to return %d results but returns %d "
               "instead.",
               expected_return_types.count, actual_return_exprs.count));
  }
}

static void analyzer_resolve_function_return(analyzer_context_t ctx,
                                             ast_node_t *fn) {
  if (fn->fn.return_types.count == 0 &&
      fn->fn.block->block.return_statement == NULL) {
    return;
  }
  // Simple case: the return statement is in the root function block
  if (fn->fn.block->block.return_statement) {
    // Set the parent to the block containing the return statement
    // so symbol table chain lookup will start at the lowest block
    ctx.parent = fn->fn.block;
    analyzer_type_check_function_return(
        ctx, fn->fn.block->block.return_statement, fn->fn.return_types,
        fn->fn.block->block.return_statement->returns.exprs);
    return;
  } else {
    ast_node_t *last_node = dynarray_get(&fn->fn.block->block.nodes,
                                         fn->fn.block->block.nodes.count - 1);
    analyzer_report_syntax_error(ctx, last_node,
                                 cstr("Missing return statement"), cstr(""));
  }
  // TODO: Eventually need to visit every child block recursively and check
  // for a return statement.  All paths must have a return.
}

// DONE: Do whole tree, not just the root node
// DONE: Error handling!
// DONE: Symbol lookup
// DONE: Function analysis  (Return analysis, is "interesting")
// DONE: If statement analysis
// TODO: High/medium level IR perhaps?
static void analyzer_resolve_types(analyzer_context_t ctx, ast_node_t *root) {
  assert(root->type == ast_block);
  symbol_table_t *current_symbol_table = root->block.symbol_table;
  ctx.parent = root;
  dynarray children = root->block.nodes;
  for (uint64_t i = 0; i < children.count; i++) {
    ast_node_t *child = dynarray_get(&children, i);
    switch (child->type) {
    case ast_assignment:
      analyze_assignment(ctx, child);
      analyze_update_symbol_table(current_symbol_table,
                                  child->assignment.identifier->symbol.value,
                                  child->assignment.type);
      break;
    case ast_block:
      analyzer_resolve_types(ctx, child);
      break;
    case ast_fn: {
      printf("analyzing function %.*s\n",
             child->fn.identifier->symbol.value.length,
             child->fn.identifier->symbol.value.ptr);
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
        analyzer_report_syntax_error(
            ctx, child->if_statement.expr, cstr("Must be boolean"),
            cstr("If statement expressions must evaluate to a boolean."));
      }
      break;
    }
    case ast_return:
    case ast_fn_call:
    case ast_bool_literal:
    case ast_float_literal:
    case ast_str_literal:
    case ast_int_literal:
    case ast_decl:
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
  analyzer_context_t ctx = {.allocator = unit->allocator,
                            .errors = unit->errors,
                            .parent = NULL,
                            .current_function = NULL};

  analyzer_resolve_types(ctx, unit->root);
  printf("Error count is %li\n", ctx.errors->count);
}
