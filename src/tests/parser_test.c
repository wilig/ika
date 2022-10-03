
#include "../../lib/allocator.h"
#include "../../lib/str.h"

#include "../parser.h"
#include "../print.h"
#include "../tokenize.h"

void test_simple_multiplication(allocator_t allocator) {
  str test_precedence_term = cstr("10 * 5 * 3 * 8 * 2");

  tokenizer_input_stream in = {
      .source = test_precedence_term, .pos = 0, .allocator = allocator};
  dynarray *tokens = tokenizer_scan(&in);
  parser_state_t state = {
      .tokens = tokens, .current_token = 0, .allocator = allocator};
  ast_node_t *root = parse_node(&state);
  printf("simple multiplication:\n");
  print_node_as_sexpr(root);
  printf("\n");
}

void test_multiplication_and_division(allocator_t allocator) {
  str test_precedence_term = cstr("10 * 5 / 8 * 27 / 2");
  tokenizer_input_stream in = {
      .source = test_precedence_term, .pos = 0, .allocator = allocator};
  dynarray *tokens = tokenizer_scan(&in);
  parser_state_t state = {
      .tokens = tokens, .current_token = 0, .allocator = allocator};
  ast_node_t *root = parse_node(&state);
  printf("multiplication and division:\n");
  print_node_as_sexpr(root);
  printf("\n");
}

void test_simple_subtraction(allocator_t allocator) {
  str test_precedence_expr = cstr("10 - 5 - 3 - 2");

  tokenizer_input_stream in = {
      .source = test_precedence_expr, .pos = 0, .allocator = allocator};
  dynarray *tokens = tokenizer_scan(&in);
  parser_state_t state = {
      .tokens = tokens, .current_token = 0, .allocator = allocator};
  ast_node_t *root = parse_node(&state);
  printf("simple subtraction:\n");
  print_node_as_sexpr(root);
  printf("\n");
}

void test_precedence_expr(allocator_t allocator) {
  str test_precedence_expr = cstr("10 - 5 * 3 - 2");

  tokenizer_input_stream in = {
      .source = test_precedence_expr, .pos = 0, .allocator = allocator};
  dynarray *tokens = tokenizer_scan(&in);
  parser_state_t state = {
      .tokens = tokens, .current_token = 0, .allocator = allocator};
  ast_node_t *root = parse_node(&state);
  printf("simple mixed expr:\n");
  print_node_as_sexpr(root);
  printf("\n");
}

void test_complex_precedence_expr(allocator_t allocator) {
  str test_precedence_expr = cstr("(10 - 5) * 3 / 2 + 50 / 4");

  tokenizer_input_stream in = {
      .source = test_precedence_expr, .pos = 0, .allocator = allocator};
  dynarray *tokens = tokenizer_scan(&in);
  parser_state_t state = {
      .tokens = tokens, .current_token = 0, .allocator = allocator};
  ast_node_t *root = parse_node(&state);
  printf("complex precedence expr:\n");
  print_node_as_sexpr(root);
  printf("\n");
}

void test_untyped_assignment_expr(allocator_t allocator) {
  str test_precedence_expr = cstr("crusty := (10 - 5) * 3 / 2 + 50 / 4");

  tokenizer_input_stream in = {
      .source = test_precedence_expr, .pos = 0, .allocator = allocator};
  dynarray *tokens = tokenizer_scan(&in);
  parser_state_t state = {
      .tokens = tokens, .current_token = 0, .allocator = allocator};
  ast_node_t *root = parse_node(&state);
  printf("untyped assignment statement:\n");
  print_node_as_tree(root, 0);
  printf("\n");
}

void test_typed_assignment_expr(allocator_t allocator) {
  str test_precedence_expr = cstr("crusty : u32 = (10 - 5) * 3 / 2 + 50 / 4");

  tokenizer_input_stream in = {
      .source = test_precedence_expr, .pos = 0, .allocator = allocator};
  dynarray *tokens = tokenizer_scan(&in);
  parser_state_t state = {
      .tokens = tokens, .current_token = 0, .allocator = allocator};
  ast_node_t *root = parse_node(&state);
  printf("typed assignment statement:\n");
  print_node_as_tree(root, 0);
  printf("\n");
}

void test_constant_typed_assignment_expr(allocator_t allocator) {
  str test_precedence_expr =
      cstr("let crusty : u32 = (10 - 5) * 3 / 2 + 50 / 4");

  tokenizer_input_stream in = {
      .source = test_precedence_expr, .pos = 0, .allocator = allocator};
  dynarray *tokens = tokenizer_scan(&in);
  parser_state_t state = {
      .tokens = tokens, .current_token = 0, .allocator = allocator};
  ast_node_t *root = parse_node(&state);
  printf("const typed assignment statement:\n");
  print_node_as_tree(root, 0);
  printf("\n");
}

int main(int argc, char **args) {
  allocator_t allocator =
      allocator_init(linear_allocator, (allocator_options){0});
  test_simple_multiplication(allocator);
  test_multiplication_and_division(allocator);
  test_simple_subtraction(allocator);
  test_precedence_expr(allocator);
  test_complex_precedence_expr(allocator);
  test_untyped_assignment_expr(allocator);
  test_typed_assignment_expr(allocator);
  test_constant_typed_assignment_expr(allocator);
}
