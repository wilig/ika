#pragma once

#include "../lib/allocator.h"
#include "../lib/str.h"

#include "errors.h"
#include "parser.h"

typedef struct scope_t {
  str name;
  int total_decls;
  struct scope_t *parent;
  struct scope_t *children;
  size_t number_of_children;
} scope_t;

typedef struct {
  char *src_file;
  char *namespace_name;

  char *buffer;
  u64 buffer_length;
  // Dynamic array
  token_t *tokens;
  int current_token_idx;

  ast_node_t *root;

  // Dynamic array
  syntax_error_t *errors;
} compilation_unit_t;

compilation_unit_t *new_compilation_unit(char *);
void compile(compilation_unit_t *);
