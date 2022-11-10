#include <string.h>

#include "ast.h"
#include "parser.h"
#include "print.h"
#include "types.h"

#include "../lib/assert.h"

void print_node_as_sexpr(ast_node_t *node) {
  if (!node) {
    printf("void");
    return;
  }
  if (node->type == ast_term) {
    printf("(");
    if (node->term.left)
      print_node_as_sexpr(node->term.left);
    printf(" %s ", token_char_map[node->term.op]);
    if (node->term.right)
      print_node_as_sexpr(node->term.right);
    printf(")");
  } else if (node->type == ast_expr) {
    printf("(");
    if (node->expr.left)
      print_node_as_sexpr(node->expr.left);
    printf(" %s ", token_char_map[node->expr.op]);
    if (node->expr.right)
      print_node_as_sexpr(node->expr.right);
    printf(")");
  } else if (node->type == ast_int_literal) {
    printf("%li", node->literal.integer_value);
  } else if (node->type == ast_float_literal) {
    printf("%fl", node->literal.float_value);
  } else if (node->type == ast_bool_literal) {
    printf("%s", node->literal.integer_value == 1 ? "true" : "false");
  } else if (node->type == ast_str_literal) {
    printf("%s", node->literal.string_value);
  } else if (node->type == ast_symbol) {
    printf("%s", node->symbol.value);
  } else if (node->type == ast_fn_call) {
    printf("(%s ", node->fn_call.symbol->symbol.value);
    for (uint32_t i = 0; i < darray_len(node->fn_call.exprs); i++) {
      ast_node_t *expr = &node->fn_call.exprs[i];
      print_node_as_sexpr(expr);
      if (i < darray_len(node->fn_call.exprs) - 1)
        printf(", ");
    }
    printf(")");
  } else {
    printf("print_node_as_sexpr: Unhandled node type: %d", node->type);
  }
}

void print_indent(uint32_t indent_level) {
  for (int i = 0; i < indent_level; i++) {
    printf("  ");
  }
}

void print_node_as_tree(ast_node_t *node, uint32_t indent_level) {
  ASSERT_MSG((node != NULL), "Tried to print a node tree for a null value");
  switch (node->type) {
  case ast_decl: {
    print_indent(indent_level);
    printf("%lc ", 0x251c);
    ast_node_t *identifier = node->decl.symbol;
    if (node->decl.constant)
      printf("[CONST] ");
    printf("%s [%s] = ", identifier->symbol.value,
           token_as_char[node->decl.type]);
    if (node->decl.expr)
      print_node_as_sexpr(node->decl.expr);
    printf("\n");
    break;
  }
  case ast_if_stmt: {
    print_indent(indent_level);
    printf("%lc ", 0x251c);
    printf("if ");
    print_node_as_sexpr(node->if_stmt.expr);
    printf("\n");
    print_indent(indent_level);
    printf("%lc ", 0x251c);
    printf("then\n");
    print_node_as_tree(node->if_stmt.if_block, indent_level);
    if (node->if_stmt.else_block) {
      print_indent(indent_level);
      printf("%lc ", 0x251c);
      printf("else\n");
      print_node_as_tree(node->if_stmt.else_block, indent_level);
    }
    break;
  }
  case ast_fn: {
    print_indent(indent_level);
    printf("%lc ", 0x251c);
    printf("fn ");
    ast_node_t *identifier = node->fn.symbol;
    printf("%s", identifier->symbol.value);
    printf("(");
    for (int i = 0; i < darray_len(node->fn.parameters); i++) {
      ast_node_t *decl_node = &node->fn.parameters[i];
      identifier = decl_node->decl.symbol;
      printf("%s:%s", identifier->symbol.value,
             token_as_char[decl_node->decl.type]);
      if (i < darray_len(node->fn.parameters) - 1)
        printf(", ");
    }
    printf(") returns ");
    printf("%s", token_as_char[node->fn.return_type]);
    printf("\n");
    print_node_as_tree(node->fn.block, indent_level);
    break;
  }
  case ast_assignment: {
    print_indent(indent_level);
    printf("%lc ", 0x251c);
    ast_node_t *identifier = node->assignment.symbol;
    printf("%s = ", identifier->symbol.value);
    print_node_as_sexpr(node->assignment.expr);
    printf("\n");
    break;
  }
  case ast_block: {
    print_indent(indent_level);
    printf("%lc%lc%lc\n", 0x2514, 0x2500, 0x2510);
    for (u64 i = 0; i < darray_len(node->block.nodes); i++) {
      da_nodes *head = node->block.nodes;
      ast_node_t *child = &head[i];
      print_node_as_tree(child, indent_level + 1);
    }
    if (node->block.return_statement) {
      print_node_as_tree(node->block.return_statement, indent_level + 1);
    }
    print_indent(indent_level);
    printf("%lc%lc%lc", 0x250c, 0x2500, 0x2518);
    printf("\n");
    break;
  }
  case ast_fn_call: {
    print_indent(indent_level);
    printf("%lc ", 0x251c);
    ast_node_t *identifier = node->fn_call.symbol;
    printf("call fn '%s' ", identifier->symbol.value);
    if (darray_len(node->fn_call.exprs) == 0) {
      printf("passing no parameters");
    } else {
      printf("passing (%li) parameters", darray_len(node->fn_call.exprs));
    }
    printf("\n");
    break;
  }
  case ast_return: {
    print_indent(indent_level);
    printf("%lc ", 0x251c);
    printf("return ");
    ast_node_t *expr = node->returns.expr;
    printf("(");
    print_node_as_sexpr(expr);
    printf(")");
    printf("\n");
    break;
  }
  default: {
    printf("I can't print a %d yet.\n", node->type);
  }
  }
}

void print_symbol_table(symbol_table_t *t) {
  printf("┌-"
         "─────────────────────────────────────┬────────────────────┬──────────"
         "─┬─────────────────┐\n");
  printf("│Name                                  │Type                │ Line   "
         "   │   Address       │\n");
  printf("├──────────────────────────────────────┼────────────────────┼────────"
         "───┼─────────────────┤\n");
  ;

  hashtbl_str_keys_t ht_keys = hashtbl_str_get_keys(t->table);
  for (u32 i = 0; i < ht_keys.count; i++) {
    symbol_table_entry_t *entry =
        symbol_table_lookup(t, str_to_cstr(*ht_keys.keys[i])); // LEAK
    printf("│ %-37s", entry->symbol);
    printf("│ %-19s", token_as_char[entry->type]);
    printf("│ %*i", 10, entry->line);
    printf("│ %*p", 16, entry->node_address);
    if (entry->constant)
      printf("│ CONSTANT\n");
    else
      printf("│\n");
  }
  printf("└──────────────────────────────────────┴────────────────────┴────────"
         "───┴─────────────────┘\n");
  ;
}
