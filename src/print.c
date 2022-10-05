#include "print.h"
#include "parser.h"

void print_node_as_sexpr(ast_node_t *node) {
  if (node->type == ast_term) {
    printf("(");
    if (node->term.left)
      print_node_as_sexpr(node->term.left);
    printf(" %s ", ika_base_type_table[node->term.op].txt);
    if (node->term.right)
      print_node_as_sexpr(node->term.right);
    printf(")");
  } else if (node->type == ast_expr) {
    printf("(");
    if (node->expr.left)
      print_node_as_sexpr(node->expr.left);
    printf(" %s ", ika_base_type_table[node->expr.op].txt);
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
    printf("%.*s", node->literal.string_value.length,
           node->literal.string_value.ptr);
  }
}

void print_indent(uint32_t indent_level) {
  for (int i = 0; i < indent_level; i++) {
    printf("  ");
  }
}

void print_node_as_tree(ast_node_t *node, uint32_t indent_level) {
  switch (node->type) {
  case ast_assignment: {
    print_indent(indent_level);
    printf("%lc ", 0x251c);
    ast_node_t *identifier = node->assignment.identifier;
    if (node->assignment.constant)
      printf("[CONST] ");
    printf("%.*s [%s] = ", identifier->symbol.value.length,
           identifier->symbol.value.ptr,
           ika_base_type_table[node->assignment.type].label);
    print_node_as_sexpr(node->assignment.expr);
    printf("\n");
    break;
  }
  case ast_if_statement: {
    print_indent(indent_level);
    printf("%lc ", 0x251c);
    printf("if ");
    print_node_as_sexpr(node->if_statement.expr);
    printf("\n");
    print_indent(indent_level);
    printf("%lc ", 0x251c);
    printf("then\n");
    print_node_as_tree(node->if_statement.if_block, indent_level);
    if (node->if_statement.else_block) {
      print_indent(indent_level);
      printf("%lc ", 0x251c);
      printf("else\n");
      print_node_as_tree(node->if_statement.else_block, indent_level);
    }
    break;
  }
  case ast_block: {
    print_indent(indent_level);
    printf("%lc%lc%lc\n", 0x2514, 0x2500, 0x2510);
    uint32_t nodes = node->block.nodes.count;
    for (int i = 0; i < nodes; i++) {
      ast_node_t *child = (ast_node_t *)dynarray_get(&node->block.nodes, i);
      print_node_as_tree(child, indent_level + 1);
    }
    print_indent(indent_level);
    printf("%lc%lc%lc", 0x250c, 0x2500, 0x2518);
    printf("\n");
    break;
  }
  default: {
    printf("I can't print a %d yet.\n", node->type);
  }
  }
}
