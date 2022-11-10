#include "qbe.h"

#include "../ast.h"
#include "../compiler.h"

#include "../rt/str_builder.h"

#include "../../lib/assert.h"
#include <stdio.h>

static b8 is_expr_or_term(ast_node_t *node) {
  return node->type == ast_expr || node->type == ast_term;
}

static u64 int_value(symbol_table_t *st, ast_node_t *node) {
  if (node->type == ast_int_literal) {
    return (u64)node->literal.integer_value;
  } else {
    ASSERT_MSG((false), "Unhandled ast_node_t in int_value");
  }
}

static f64 float_value(symbol_table_t *st, ast_node_t *node) {
  if (node->type == ast_float_literal) {
    return (f64)node->literal.float_value;
  } else {
    ASSERT_MSG((false), "Unhandled ast_node_t in int_value");
  }
}

static char ika_type_to_qbe_type(e_token_type ika_type) {
  switch (ika_type) {
  case TOKEN_INT:
    return 'l';
  case TOKEN_FLOAT:
    return 'd';
  case TOKEN_STR:
    return 'b';
  case TOKEN_BOOL:
    return 'b';
  default: {
    ASSERT_MSG((false), "Unsupported ika->qbe type conversion");
  }
  }
}

static u32 sub_expression(symbol_table_t *st, ast_node_t *node,
                          e_token_type type, u32 regnum, u32 *maxreg) {
  char expr[250], lhs[100], rhs[100];
  char type_spec = type == TOKEN_INT ? 'l' : 'd';
  ASSERT_MSG((node->type == ast_expr || node->type == ast_term),
             "Expected an expression or term");
  ast_node_t *left = node->type == ast_expr ? node->expr.left : node->term.left;
  ast_node_t *right =
      node->type == ast_expr ? node->expr.right : node->term.right;
  e_token_type op = node->type == ast_expr ? node->expr.op : node->term.op;
  if (regnum < *maxreg) {
    regnum = *maxreg + 1;
    *maxreg += 1;
  } else if (regnum > *maxreg) {
    *maxreg = regnum;
  }
  if (is_expr_or_term(left)) {
    u32 i = sub_expression(st, left, type, regnum + 1, maxreg);
    snprintf(lhs, 100, "%c %%r%d", type_spec, i);
  } else {
    if (type == TOKEN_INT)
      snprintf(lhs, 100, "%c %lu", type_spec, int_value(st, left));
    else
      snprintf(lhs, 100, "%c %f", type_spec, float_value(st, left));
  }
  if (is_expr_or_term(right)) {
    u32 i = sub_expression(st, right, type, regnum + 1, maxreg);
    snprintf(rhs, 100, "%c %%r%d", type_spec, i);
  } else {
    if (type == TOKEN_INT)
      snprintf(rhs, 100, "%c %lu", type_spec, int_value(st, right));
    else
      snprintf(rhs, 100, "%c %f", type_spec, float_value(st, right));
  }
  switch (op) {
  case TOKEN_ADD:
    snprintf(expr, 250, "%%r%d =%c add %s, %s", regnum, type_spec, lhs, rhs);
    printf("%s\n", expr);
    break;
  case TOKEN_SUB:
    snprintf(expr, 250, "%%r%d =%c sub %s, %s", regnum, type_spec, lhs, rhs);
    printf("%s\n", expr);
    break;
  case TOKEN_MUL:
    snprintf(expr, 250, "%%r%d =%c mul %s, %s", regnum, type_spec, lhs, rhs);
    printf("%s\n", expr);
    break;
  case TOKEN_QUO:
    snprintf(expr, 250, "%%r%d =%c div %s, %s", regnum, type_spec, lhs, rhs);
    printf("%s\n", expr);
    break;
  default: {
    printf("Operator not yet implemented: %d\n", node->expr.op);
    ASSERT(FALSE);
  }
  }
  return regnum;
}

static void build_expression(symbol_table_t *st, ast_node_t *node) {
  ASSERT_MSG((node->type == ast_expr || node->type == ast_term),
             "Expected an expression or term");
  u32 max_reg = 1;
  sub_expression(st, node, TOKEN_INT, 1, &max_reg);
}

/* static void build_function(str_builder_t sb, fn_t *fn) { */
/*   char *name = fn->symbol->symbol.value; */
/*   char qbe_return_type = ika_type_to_qbe_type(fn->return_type); */
/*   str_builder_append(sb, "function "); */
/*   str_builder_append(sb, qbe_return_type); */
/*   str_builder_append(sb, " $"); */
/*   str_builder_append(sb, name); */
/*   str_builder_append(sb, "("); */
/*   if (darray_len(fn->parameters) > 0) { */
/*     for (i64 i = 0; i < darray_len(fn->parameters); i++) { */
/*       char qbe_param_type =
 * ika_type_to_qbe_type(fn->parameters[i].decl.type); */
/*       str_builder_append(sb, qbe_param_type); */
/*       str_builder_append(sb, " %p"); */
/*       str_builder_append(sb, i); */
/*       if (i < darray_len(fn->parameters)) */
/*         str_builder_append(sb, ", "); */
/*     } */
/*   } */
/*   str_builder_append(sb, ") {\n@start\n"); */
/*   // Build block */
/*   str_builder_append(sb, "}\n"); */
/* } */

b8 qbe_generate(compilation_unit_t *unit) {
  printf("Generating code for qbe backend\n");
  symbol_table_t *myst = unit->root->block.symbol_table;
  for (int i = 0; i < darray_len(unit->root->block.nodes); i++) {
    ast_node_t *child = &unit->root->block.nodes[i];
    printf("Child node is: %d\n", child->type);
    if (child->type == ast_decl) {
      build_expression(myst, child->decl.expr);
    }
  }
  return FALSE;
}
