#include <assert.h>

#include "ast.h"
#include "compiler.h"
#include "symbol_table.h"
#include "types.h"

/* #include <assert.h> */
/* #include <stdlib.h> */

/* #include "../lib/str.h" */

/* #include "analyzer.h" */
/* #include "parser.h" */
/* #include "symtbl.h" */
/* #include "tokenize.h" */
/* #include "types.h" */

/* bool starts_with_digit(str value) { */
/*   return value.ptr[0] >= '0' && value.ptr[0] <= 9; */
/* } */

/* e_ika_type specialize_num_literal(str value) { */
/*   if (str_contains(value, cstr("."))) { */
/*     return ika_float_literal; */
/*   } else { */
/*     return ika_int_literal; */
/*   } */
/* } */

/* e_ika_type analyzer_type_a_binary_expr(symtbl_t *t, expr_t *expr); */

/* e_ika_type analyzer_deduce_type(symtbl_t *t, expr_t *expr) { */
/*   switch (expr->type) { */
/*   case literal_value: { */
/*     literal_value_t lit = expr->literal; */
/*     if (lit.type == ika_num_literal) { */
/*       printf("trying to specialize a num literal\n"); */
/*       return specialize_num_literal(lit.value->value); */
/*     } else { */
/*       printf("couldn't do anything with %s\n", */
/*              tokenizer_get_token_type_name(lit.type).ptr); */
/*       return lit.type; */
/*     } */
/*     break; */
/*   } */
/*   case identifier: { */
/*     str ident = expr->identifier.value->value; */
/*     symtbl_entry_t *entry = symtbl_lookup(t, ident); */
/*     if (entry) { */
/*       return entry->type; */
/*     } else { */
/*       return ika_unknown; */
/*     } */
/*     break; */
/*   } */
/*   case binary_expr: { */
/*     return analyzer_type_a_binary_expr(t, expr); */
/*     break; */
/*   } */
/*   } */
/* } */

/* e_ika_type analyzer_type_a_binary_expr(symtbl_t *t, expr_t *expr) { */
/*   assert(expr->type == binary_expr); */
/*   binary_expr_t binary_expr = expr->binary; */
/*   e_ika_type left_type = analyzer_deduce_type(t, binary_expr.left); */
/*   e_ika_type right_type = analyzer_deduce_type(t, binary_expr.right); */
/*   if (left_type == right_type) { */
/*     return left_type; */
/*   } else { */
/*     printf("Type mismatch left side is %s type, right side is %s type.\n", */
/*            tokenizer_get_token_type_name(left_type).ptr, */
/*            tokenizer_get_token_type_name(right_type).ptr); */
/*     exit(-1); */
/*   } */
/* } */

/* void analyzer_update_type(symtbl_t *t, str ident, e_ika_type type) { */
/*   symtbl_entry_t *entry = symtbl_lookup(t, ident); */
/*   entry->type = type; */
/* } */

e_ika_type determine_type_for_expression(ast_node_t *expression) {
  switch (expression->type) {
  case ast_int_literal:
    return ika_int;
  case ast_float_literal:
    return ika_float;
  case ast_bool_literal:
    return ika_bool;
  case ast_str_literal:
    return ika_str;
  case ast_symbol:
    // TODO: Look this up in the symbol table
    return ika_unknown;
  case ast_term:
  case ast_expr: {
    e_ika_type ltype = determine_type_for_expression(expression->expr.left);
    e_ika_type rtype = determine_type_for_expression(expression->expr.right);
    if (ltype == rtype) { // Simple case
      return ltype;
    } else if (ltype == ika_int && rtype == ika_float ||
               ltype == ika_float && rtype == ika_int) {
      return ika_float;
    } else {
      printf("%s %s %s makes no sense, at line %d\n",
             ika_base_type_table[ltype].txt,
             ika_base_type_table[expression->expr.op].txt,
             ika_base_type_table[rtype].txt, expression->line);
      exit(-1);
    }
  }
  default: {
    assert(false);
  }
  }
}

void analyze_assignment(ast_node_t *node) {
  printf("Analyzing assignment %.*s, type is ",
         node->assignment.identifier->symbol.value.length,
         node->assignment.identifier->symbol.value.ptr);
  e_ika_type type = determine_type_for_expression(node->assignment.expr);
  printf("%s\n", ika_base_type_table[type].txt);
  if (node->assignment.type == ika_untyped_assign) {
    node->assignment.type = type;
  } else if (node->assignment.type != type) {
    printf("Type mismatch!  Line: %d\n", node->line);
    exit(-1);
  }
}

void analyzer_resolve_types(ast_node_t *root) {
  assert(root->type == ast_block);
  dynarray children = root->block.nodes;
  symbol_table_t *symbol_table = root->block.symbol_table;
  for (int i = 0; i < children.count; i++) {
    ast_node_t *child = dynarray_get(&children, i);
    switch (child->type) {
    case ast_assignment: {
      analyze_assignment(child);
      break;
    }
    default:
      break;
    }
  }
}

void analyzer_analyze(compilation_unit_t *unit) {
  printf("Analyzing types\n");
  analyzer_resolve_types(unit->root);
}
