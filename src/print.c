#include "print.h"
#include "parser.h"
#include "types.h"

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
  } else if (node->type == ast_symbol) {
    printf("%.*s", node->symbol.value.length, node->symbol.value.ptr);
  } else {
    printf("Unhandled node type: %d", node->type);
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
  case ast_fn: {
    print_indent(indent_level);
    printf("%lc ", 0x251c);
    printf("fn ");
    ast_node_t *identifier = node->fn.identifier;
    printf("%.*s", identifier->symbol.value.length,
           identifier->symbol.value.ptr);
    printf("(");
    for (int i = 0; i < node->fn.parameters.count; i++) {
      ast_node_t *decl_node = dynarray_get(&node->fn.parameters, i);
      identifier = decl_node->decl.identifier;
      printf("%.*s:%s", identifier->symbol.value.length,
             identifier->symbol.value.ptr,
             ika_base_type_table[decl_node->decl.type].label);
      if (i < node->fn.parameters.count - 1)
        printf(", ");
    }
    printf(") returns ");
    if (node->fn.return_types.count == 0) {
      printf("void");
    } else {
      printf("(");
      for (int i = 0; i < node->fn.return_types.count; i++) {
        e_ika_type *return_type = dynarray_get(&node->fn.return_types, i);
        printf("%s", ika_base_type_table[*return_type].label);
        if (i < node->fn.return_types.count - 1)
          printf(", ");
      }
      printf(")");
    }
    printf("\n");
    print_node_as_tree(node->fn.block, indent_level);
    break;
  }
  case ast_decl: {
    print_indent(indent_level);
    printf("%lc ", 0x251c);
    ast_node_t *identifier = node->decl.identifier;
    printf("%.*s [%s]", identifier->symbol.value.length,
           identifier->symbol.value.ptr,
           ika_base_type_table[node->decl.type].label);
    printf("\n");
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
    ast_node_t *identifier = node->fn_call.identifer;
    printf("call fn '%.*s' ", identifier->symbol.value.length,
           identifier->symbol.value.ptr);
    if (node->fn_call.exprs.count == 0) {
      printf("passing no parameters");
    } else {
      printf("passing (%li) parameters", node->fn_call.exprs.count);
    }
    printf("\n");
    break;
  }
  case ast_return: {
    print_indent(indent_level);
    printf("%lc ", 0x251c);
    printf("return ");
    dynarray exprs = node->returns.exprs;
    if (exprs.count > 0) {
      printf("(");
      for (int i = 0; i < exprs.count; i++) {
        print_node_as_sexpr(dynarray_get(&exprs, i));
        if (i < exprs.count - 1)
          printf(", ");
      }
      printf(")");
    }
    printf("\n");
    break;
  }
  default: {
    printf("I can't print a %d yet.\n", node->type);
  }
  }
}
