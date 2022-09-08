#include "debug.h"
#include "hashtbl.h"
#include "symtbl.h"
#include "tokenize.h"

void debug_print_parsed_scope(compilation_unit_t *, scope_t *);

void debug_print_parsed_expr(compilation_unit_t *unit, expr_t *expr) {
  switch (expr->type) {
  case literal_value: {
    literal_value_t *lv = &expr->literal;
    printf("%.*s", lv->value->value.length, lv->value->value.ptr);
    printf(" <LiteralValue> ");
    break;
  }
  case identifier: {
    identifier_t *i = &expr->identifier;
    printf("%.*s", i->value->value.length, i->value->value.ptr);
    printf(" <Identifier> ");
    break;
  }
  case binary_expr: {
    binary_expr_t *be = &expr->binary;
    debug_print_parsed_expr(unit, be->left);
    printf("%.*s", be->op->value.length, be->op->value.ptr);
    printf(" <Operator> ");
    debug_print_parsed_expr(unit, be->right);
    break;
  }
  }
}

void debug_print_parsed_stmt(compilation_unit_t *unit, stmt_t *stmt) {
  switch (stmt->type) {
  case let_statement: {
    let_stmt_t *ls = &stmt->let_statement;
    printf("LET ");
    printf("%.*s", ls->identifier->value.length, ls->identifier->value.ptr);
    printf(" = ");
    debug_print_parsed_expr(unit, ls->expr);
    printf("\n");
    break;
  }
  case var_statement: {
    var_stmt_t *vs = &stmt->var_statement;
    printf("VAR ");
    printf("%.*s", vs->identifier->value.length, vs->identifier->value.ptr);
    printf(" = ");
    debug_print_parsed_expr(unit, vs->expr);
    printf("\n");
    break;
  }
  case assignment_statement: {
    assignment_stmt_t *as = &stmt->assignment_statement;
    printf("%.*s", as->identifier->value.length, as->identifier->value.ptr);
    printf(" ");
    printf("%.*s", as->op->value.length, as->op->value.ptr);
    printf(" ");
    debug_print_parsed_expr(unit, as->expr);
    printf("\n");
    break;
  }
  case namespace_statement: {
    namespace_stmt_t *ns = &stmt->namespace_statement;
    printf("NS ");
    printf("%.*s", ns->nameToken->value.length, ns->nameToken->value.ptr);
    printf("\n");
    break;
  }
  case import_statement: {
    printf("Import statement printing is unimplemented\n");
    break;
  }
  case if_statement: {
    if_stmt_t *fi = &stmt->if_statement;
    printf("IF ");
    debug_print_parsed_expr(unit, fi->conditional);
    debug_print_parsed_scope(unit, fi->if_scope);
    if (fi->else_scope != NULL) {
      printf("ELSE ");
      debug_print_parsed_scope(unit, fi->else_scope);
    }
  }
  }
}

void debug_print_parsed_scope(compilation_unit_t *unit, scope_t *scope) {
  printf("\nFILE: %s\n", unit->src_file.ptr);
  printf("SCOPE: %s\n", scope->name.ptr);
  printf("Symbol Table: \n");
  symtbl_dump(scope->symbol_table);
  printf("{\n");
  for (int i = 0; i < scope->total_decls; i++) {
    debug_print_parsed_stmt(unit, scope->decls[i]);
  }
  printf("}\n");
}

void debug_print_parse_tree(compilation_unit_t *unit) {
  printf("Printing parse tree\n");

  printf("My namespace is: %.*s\n", unit->namespace_name->length,
         unit->namespace_name->ptr);
  printf("\n\n");
  printf("scope %p\n", (void *)unit->scopes);
  debug_print_parsed_scope(unit, &unit->scopes[0]);
}
