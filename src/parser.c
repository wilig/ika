#include "parser.h"
#include "allocator.h"
#include "hashtbl.h"
#include "log.h"
#include "symtbl.h"
#include "tokenize.h"
#include "types.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void new_scope(allocator_t, str name, scope_t *, scope_t *);

// TODO: Move to passing the stmt/expression to the function
// Forward declaration
scope_t *parse_scope(compilation_unit_t *, str);
void parser_print_scope(compilation_unit_t *, scope_t *);

typedef bool (*type_tester_ptr_t)(e_ika_type);

void parse_expression(compilation_unit_t *, expr_t *);
void parse_simple_expression(compilation_unit_t *, expr_t *);

token_t *get_token(compilation_unit_t *unit) {
  return unit->tokens[unit->current_token_idx++];
}

void rewind_tokens(compilation_unit_t *unit, int i) {
  unit->current_token_idx -= i;
}

void skip_tokens(compilation_unit_t *unit, int i) {
  unit->current_token_idx += i;
}

token_t *peek_current_token(compilation_unit_t *unit) {
  return unit->tokens[unit->current_token_idx];
}

token_t *peek_next_token(compilation_unit_t *unit) {
  return unit->tokens[unit->current_token_idx + 1];
}

bool a_colon(e_ika_type type) { return type == ika_colon; }
bool a_semi_colon(e_ika_type type) { return type == ika_semi_colon; }

bool an_untyped_assign(e_ika_type type) { return type == ika_untyped_assign; }

bool an_assign(e_ika_type type) { return type == ika_assign; }

bool an_equal_sign(e_ika_type type) { return type == ika_eql; }

bool an_operator(e_ika_type type) {
  return type > __ika_operators_start && type < __ika_operators_end;
}

bool a_type(e_ika_type type) {
  return type > __ika_types_start && type < __ika_types_end;
}

bool a_literal(e_ika_type type) {
  return type > __ika_literal_start && type < __ika_literal_end;
}

bool an_identifer(e_ika_type type) { return type == ika_identifier; }

bool is(type_tester_ptr_t fn, token_t *t) { return fn(t->type); }

void expect_and_consume(compilation_unit_t *unit, e_ika_type expected_type) {
  if (get_token(unit)->type != expected_type) {
    log_error("expected type: {s}, got type: {s}\n",
              tokenizer_get_token_type_name(expected_type),
              tokenizer_get_token_type_name(get_token(unit)->type));
    assert(false);
  }
}

bool conform(compilation_unit_t *unit, type_tester_ptr_t funs[], size_t total) {
  for (int i = 0; i < total; i++) {
    if (!(funs[i])(get_token(unit)->type)) {
      rewind_tokens(unit, i + 1);
      return false;
    }
  }
  rewind_tokens(unit, total);
  return true;
}

bool is_one_of(e_ika_type t, e_ika_type to_check[]) {
  int total = sizeof(&to_check) / sizeof(t);
  for (int i = 0; i < total; i++) {
    if (t == to_check[i])
      return true;
  }
  return false;
}

expr_t *new_expr(allocator_t allocator) {
  return allocator_alloc_or_exit(allocator, sizeof(expr_t));
}

stmt_t *make_stmt(allocator_t allocator) {
  return allocator_alloc_or_exit(allocator, sizeof(stmt_t));
}

static void parse_literal(compilation_unit_t *unit, expr_t *expr) {
  token_t *t = get_token(unit);
  assert(a_literal(t->type));
  expr->type = literal_value;
  expr->literal.start_token = unit->current_token_idx;
  expr->literal.end_token = unit->current_token_idx;
  expr->literal.type = t->type;
  expr->literal.value = t;
}

static void parse_identifier(compilation_unit_t *unit, expr_t *expr) {
  token_t *t = get_token(unit);
  assert(t->type == ika_identifier);
  expr->type = identifier;
  expr->identifier.start_token = unit->current_token_idx;
  expr->identifier.end_token = unit->current_token_idx;
  expr->identifier.type = t->type;
  expr->identifier.value = t;
}

static void parse_binary(compilation_unit_t *unit, expr_t *expr) {
  expr->type = binary_expr;
  expr->binary.start_token = unit->current_token_idx;

  expr->binary.left = new_expr(unit->allocator);
  parse_simple_expression(unit, expr->binary.left);

  expr->binary.op = get_token(unit);
  assert(expr->binary.op->type > __ika_operators_start &&
         expr->binary.op->type < __ika_operators_end);

  expr->binary.right = new_expr(unit->allocator);
  parse_expression(unit, expr->binary.right);

  expr->binary.end_token = unit->current_token_idx;
}

void parse_simple_expression(compilation_unit_t *unit, expr_t *expr) {
  token_t *token = peek_current_token(unit);

  if (a_literal(token->type) == true) {
    printf("parsing simple literal\n");
    parse_literal(unit, expr);
  } else if (an_identifer(token->type)) {
    printf("parsing simple identifier\n");
    parse_identifier(unit, expr);
  } else {
    printf(
            "Was expecting a literal, identifer, or binary expression got a %s\n",
            tokenizer_get_token_type_label(token).ptr);
    assert(false);
  }
}

void parse_expression(compilation_unit_t *unit, expr_t *expr) {
  token_t *nt = peek_next_token(unit);
  token_t *token = peek_current_token(unit);

  if (an_operator(nt->type)) {
    printf("parsing binary expression\n");
    parse_binary(unit, expr);
  } else {
    parse_simple_expression(unit, expr);
  }
}

void parse_namespace_stmt(compilation_unit_t *unit, stmt_t *stmt) {
  token_t *ns_token = get_token(unit);

  // Sanity check
  assert(ns_token != NULL);
  assert(ns_token->type == ika_keyword_ns);
  assert(peek_current_token(unit)->type == ika_identifier);

  stmt->type = namespace_statement;
  stmt->namespace_statement.start_token = unit->current_token_idx;
  stmt->namespace_statement.nameToken = get_token(unit);

  expect_and_consume(unit, ika_semi_colon);

  stmt->namespace_statement.end_token = unit->current_token_idx;
}

void parse_assignment_stmt(compilation_unit_t *unit, stmt_t *stmt) {
  type_tester_ptr_t assignment[] = {&an_identifer, &an_operator};
  assert(conform(unit, assignment, 2));

  stmt->type = assignment_statement;
  stmt->assignment_statement.start_token = unit->current_token_idx;
  stmt->assignment_statement.identifier = get_token(unit);
  stmt->assignment_statement.op = get_token(unit);
  stmt->assignment_statement.expr = new_expr(unit->allocator);
  parse_expression(unit, stmt->assignment_statement.expr);
  expect_and_consume(unit, ika_semi_colon);
}

void parse_let_stmt(compilation_unit_t *unit, stmt_t *stmt) {
  token_t *let_token = get_token(unit);
  // Sanity checks
  assert(let_token != NULL);
  assert(let_token->type == ika_keyword_let);

  stmt->type = let_statement;
  stmt->let_statement.start_token =
      unit->current_token_idx - 1; // include the let token_t
  stmt->let_statement.expr = new_expr(unit->allocator);

  // Two possible let syntax variants
  type_tester_ptr_t inferred[] = {&an_identifer, &an_untyped_assign};
  type_tester_ptr_t explicitly_typed[] = {&an_identifer, &a_colon, &a_type,
                                          &an_assign};
  if (conform(unit, inferred, 2)) {
    stmt->let_statement.identifier = get_token(unit);
    stmt->let_statement.explicit_type = NULL;
    expect_and_consume(unit, ika_untyped_assign);
    parse_expression(unit, stmt->let_statement.expr);
    expect_and_consume(unit, ika_semi_colon);
  } else if (conform(unit, explicitly_typed, 4)) {
    stmt->let_statement.identifier = get_token(unit);
    expect_and_consume(unit, ika_colon);
    stmt->let_statement.explicit_type = get_token(unit);
    expect_and_consume(unit, ika_assign);
    parse_expression(unit, stmt->let_statement.expr);
    expect_and_consume(unit, ika_semi_colon);
  } else {
    printf("Failed to parse let statement.  Statement did not conform to "
           "expectations\n");
    exit(-1);
  }
  // Insert constant identifier into symbol table
    symtbl_insert(unit->current_scope->symbol_table,
                  stmt->let_statement.identifier->value,
                  stmt->let_statement.explicit_type == NULL
                  ? ika_unknown
                  : stmt->let_statement.explicit_type->type,
                  true, let_token->position.line, let_token->position.column);
}

void parse_var_stmt(compilation_unit_t *unit, stmt_t *stmt) {
  token_t *var_token = get_token(unit);
  // Sanity checks
  assert(var_token != NULL);
  assert(var_token->type == ika_keyword_var);

  stmt->type = var_statement;
  stmt->var_statement.start_token =
      unit->current_token_idx - 1; // include the let token_t
  stmt->var_statement.expr = new_expr(unit->allocator);

  // Two possible var syntax variants
  type_tester_ptr_t inferred[] = {&an_identifer, &an_untyped_assign};
  type_tester_ptr_t explicitly_typed[] = {&an_identifer, &a_colon, &a_type,
                                          &an_assign};
  if (conform(unit, inferred, 2)) {
    stmt->var_statement.identifier = get_token(unit);
    stmt->var_statement.explicit_type = NULL;
    expect_and_consume(unit, ika_untyped_assign);
    parse_expression(unit, stmt->var_statement.expr);
    expect_and_consume(unit, ika_semi_colon);
  } else if (conform(unit, explicitly_typed, 4)) {
    stmt->var_statement.identifier = get_token(unit);
    expect_and_consume(unit, ika_colon);
    stmt->var_statement.explicit_type = get_token(unit);
    expect_and_consume(unit, ika_assign);
    parse_expression(unit, stmt->var_statement.expr);
    expect_and_consume(unit, ika_semi_colon);
  } else {
    printf("Failed to parse var statement.  Statement did not conform to "
           "expectations\n");
    exit(-1);
  }
  // Insert variable identifier into symbol table
    symtbl_insert(unit->current_scope->symbol_table,
                  stmt->var_statement.identifier->value,
                  stmt->var_statement.explicit_type == NULL
                  ? ika_unknown
                  : stmt->var_statement.explicit_type->type,
                  false, var_token->position.line, var_token->position.column);
}

void parse_if_stmt(compilation_unit_t *unit, stmt_t *stmt) {
  token_t *if_token = get_token(unit);
  // Sanity checks
  assert(if_token != NULL);
  assert(if_token->type == ika_keyword_if);
  stmt->type = if_statement;
  stmt->if_statement.conditional = new_expr(unit->allocator);
  stmt->if_statement.start_token =
      unit->current_token_idx - 1; // Include the if keyword
  expect_and_consume(unit, ika_paren_open);
  parse_expression(unit, stmt->if_statement.conditional);
  expect_and_consume(unit, ika_paren_close);
  stmt->if_statement.if_scope = parse_scope(unit, cstr("if-then-scope"));
  if (peek_current_token(unit)->type == ika_keyword_else) {
    expect_and_consume(unit, ika_keyword_else);
    stmt->if_statement.else_scope = parse_scope(unit, cstr("if-else-scope"));
  }
  stmt->if_statement.end_token = unit->current_token_idx;
}

void push_scope(compilation_unit_t *unit, str name) {
  scope_t *scope = allocator_alloc_or_exit(unit->allocator, sizeof(scope_t));
  if (unit->current_scope == NULL) {
    // Set root scope
    new_scope(unit->allocator, name, NULL, scope);
  } else {
    new_scope(unit->allocator, name, unit->current_scope, scope);
  }
  unit->current_scope = scope;
}

void pop_scope(compilation_unit_t *unit) {
  unit->current_scope = unit->current_scope->parent;
}

// TODO - need variable assignment expression as first class decl;
//
scope_t *parse_scope(compilation_unit_t *unit, str name) {
  push_scope(unit, name);
  scope_t *scope = unit->current_scope;
  if (scope->parent != NULL) { // If not root scope
    expect_and_consume(unit, ika_brace_open);
  }
  int total_decls = 0;
  allocated_memory decls_mem =
      allocator_alloc(unit->allocator, sizeof(stmt_t *) * 10);
  if (!decls_mem.valid) {
    log_error("Failed to allocate memory for declarations\n");
    exit(-1);
  }
  scope->decls = decls_mem.ptr;
  while (peek_current_token(unit)->type != ika_brace_close &&
         peek_current_token(unit)->type != ika_eof) {
    switch (peek_current_token(unit)->type) {
    case ika_keyword_let: {
      printf("Parsing let statement\n");
      stmt_t *let_stmt = make_stmt(unit->allocator);
      parse_let_stmt(unit, let_stmt);
      scope->decls[total_decls++] = let_stmt;
      break;
    }
    case ika_keyword_var: {
      printf("Parsing var statement\n");
      stmt_t *var_stmt = make_stmt(unit->allocator);
      parse_var_stmt(unit, var_stmt);
      scope->decls[total_decls++] = var_stmt;
      break;
    }
    case ika_keyword_ns: {
      if (scope->parent != NULL) {
        printf("Namespace only allow at root level");
        assert(false);
      }
      printf("Parsing namespace\n");
      stmt_t *namespace_stmt = make_stmt(unit->allocator);
      parse_namespace_stmt(unit, namespace_stmt);
      printf("Set namespace name\n");
      unit->namespace_name =
          &namespace_stmt->namespace_statement.nameToken->value;
      scope->decls[total_decls++] = namespace_stmt;
      printf("Done parseing namespace\n");
      break;
    }
    case ika_keyword_if: {
      printf("Parsing if statement\n");
      stmt_t *if_stmt = make_stmt(unit->allocator);
      parse_if_stmt(unit, if_stmt);
      scope->decls[total_decls++] = if_stmt;
      break;
    }
    case ika_identifier: {
      if (an_operator(peek_next_token(unit)->type)) {
        printf("Parsing assignment statement\n");
        stmt_t *assignment_stmt = make_stmt(unit->allocator);
        parse_assignment_stmt(unit, assignment_stmt);
        scope->decls[total_decls++] = assignment_stmt;
        break;
      }
    }
    case ika_comment: {
      get_token(unit); // Skip the comment
      break;
    }
    default: {
      printf("Invalid root level delcaration, got token_t->type: %s\n",
             tokenizer_get_token_type_label(peek_current_token(unit)).ptr);
      exit(-1);
    }
    }
    if (total_decls % 10 == 0) {
      decls_mem = allocator_realloc(unit->allocator, decls_mem,
                                    (total_decls + 10) * sizeof(stmt_t *));
      if (!decls_mem.valid) {
        log_error("Failed to realloc memory for declarations\n");
      }
      scope->decls = decls_mem.ptr;
    }
  }
  if (peek_current_token(unit)->type == ika_brace_close)
    expect_and_consume(unit, ika_brace_close);
  scope->total_decls = total_decls;

  pop_scope(unit);
  return scope;
}

void parser_parse(compilation_unit_t *unit) {
  unit->scopes = parse_scope(unit, cstr("root"));
  printf("Parsing complete\n");
}

void new_scope(allocator_t allocator, str name, scope_t *parent_scope,
               scope_t *scope) {
  scope->name = name;
  scope->symbol_table = symtbl_init(allocator, scope);
  scope->number_of_children = 0;
  // TODO: Find a better way
  scope->children = allocator_alloc_or_exit(allocator, sizeof(scope_t *) * 10);
  if (parent_scope != NULL) {
    // Add this scope as a child
    scope->parent = parent_scope;
    parent_scope->children[parent_scope->number_of_children++] = *scope;
  }
}

void new_compilation_unit(allocator_t allocator, char *filename, str *buffer,
                          token_t **tokens, compilation_unit_t *unit) {
  allocated_memory mem = allocator_alloc(allocator, sizeof(scope_t));
  if (!mem.valid) {
    log_error("Failed to allocate memory, bailing..");
    exit(-1);
  }
  unit->allocator = allocator;
  unit->src_file = cstr(filename);
  unit->buffer = buffer;
  unit->tokens = tokens;
  unit->current_scope = NULL;
}
