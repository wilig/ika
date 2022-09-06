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

void fill_space(int num_spaces) {
  for (int i = 0; i < num_spaces; i++) {
    printf(" ");
  }
}

void debug_print_parsed_scope(compilation_unit_t *unit, scope_t *scope) {
  printf("\nFILE: %s\n", unit->src_file.ptr);
  printf("SCOPE: %s\n", scope->name.ptr);
  printf("Symbol Table: \n");
  printf("--------------------------------------------------------------------"
         "----------\n");
  printf("| Name                                 | Type               | Line  "
         "| Column |\n");
  printf("|--------------------------------------|--------------------|-------"
         "|--------|\n");

  hashtbl_str_keys_t ht_keys = hashtbl_str_get_keys(scope->symbol_table->table);
  for (int i = 0; i < ht_keys.count; i++) {
    symbol_table_entry_t *entry =
        symbol_table_lookup(scope->symbol_table, *ht_keys.keys[i]);
    printf("| %.*s", ht_keys.keys[i]->length, ht_keys.keys[i]->ptr);
    fill_space(37 - ht_keys.keys[i]->length);
    str type_name = tokenize_get_token_type_name(entry->type);
    printf("| %s", type_name.ptr);
    fill_space(19 - type_name.length);
    printf("| %li", entry->line);
    fill_space(6 - ((entry->line / 10000) + (entry->line / 1000) +
                    (entry->line / 100) + (entry->line / 10) + 1));
    printf("| %li", entry->column);
    fill_space(7 - ((entry->column / 10000) + (entry->column / 1000) +
                    (entry->column / 100) + (entry->column / 10) + 1));
    printf("|\n");
  }
  printf("--------------------------------------------------------------------"
         "----------\n\n");

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
