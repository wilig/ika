#include <assert.h>
#include <stdlib.h>

#include "../lib/str.h"

#include "analyzer.h"
#include "parser.h"
#include "symtbl.h"
#include "tokenize.h"
#include "types.h"

bool starts_with_digit(str value) {
  return value.ptr[0] >= '0' && value.ptr[0] <= 9;
}

e_ika_type specialize_num_literal(str value) {
  if (str_contains(value, cstr("."))) {
    return ika_float_literal;
  } else {
    return ika_int_literal;
  }
}

e_ika_type analyzer_type_a_binary_expr(symtbl_t *t, expr_t *expr);

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

/* void analyzer_resolve_types(scope_t *scope) { */
/*   printf("Analyzing scope: %.*s with %d declarations\n", scope->name.length,
 */
/*          scope->name.ptr, scope->total_decls); */
/*   // TODO: Rename total_decls to decl_count */
/*   for (int x = 0; x < scope->total_decls; x++) { */
/*     stmt_t *stmt = scope->decls[x]; */
/*     switch (stmt->type) { */
/*     case let_statement: { */
/*       let_stmt_t let = stmt->let_statement; */
/*       e_ika_type deduced_type = */
/*           analyzer_deduce_type(scope->symbol_table, let.expr); */
/*       analyzer_update_type(scope->symbol_table, let.identifier->value, */
/*                            deduced_type); */
/*       break; */
/*     } */
/*     case var_statement: { */
/*       var_stmt_t var = stmt->var_statement; */
/*       e_ika_type deduced_type = */
/*           analyzer_deduce_type(scope->symbol_table, var.expr); */
/*       analyzer_update_type(scope->symbol_table, var.identifier->value, */
/*                            deduced_type); */
/*       break; */
/*     } */
/*     default: { */
/*       printf("stmt->type = %d\n", stmt->type); */
/*       break; */
/*     } */
/*     } */
/*   } */
/* } */

void analyzer_analyze(compilation_unit_t *unit) {
  printf("Analyzing scopes\n");
  // analyzer_resolve_types(&unit->scopes[0]);
}
