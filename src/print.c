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
  } else if (node->type == ast_str_literal) {
    printf("%*.s", node->literal.string_value.length,
           node->literal.string_value.ptr);
  }
}

void print_node_as_tree(ast_node_t *node, uint32_t indent_level) {
  for (int i = 0; i < indent_level; i++) {
    printf("  ");
  }
  switch (node->type) {
  case ast_assignment: {
    ast_node_t *identifier = node->assignment.identifier;
    if (node->assignment.constant)
      printf("const ");
    printf("%.*s [%s] = ", identifier->symbol.value.length,
           identifier->symbol.value.ptr,
           ika_base_type_table[node->assignment.type].label);
    print_node_as_sexpr(node->assignment.expr);
    printf("\n");
    break;
  }
  case ast_block: {
    printf("\\\n");
    uint32_t nodes = node->block.nodes.count;
    for (int i = 0; i < nodes; i++) {
      ast_node_t *child = (ast_node_t *)dynarray_get(&node->block.nodes, i);
      print_node_as_tree(child, indent_level + 1);
    }
    printf("\n");
    break;
  }
  default: {
    printf("I can't print a %d yet.\n", node->type);
  }
  }
}
