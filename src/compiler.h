#pragma once

#include "../lib/allocator.h"
#include "../lib/dynarray.h"
#include "../lib/str.h"

#include "parser.h"

typedef struct scope_t {
  allocator_t allocator;
  str name;
  int total_decls;
  struct scope_t *parent;
  struct scope_t *children;
  size_t number_of_children;
} scope_t;

typedef struct {
  allocator_t allocator;
  char *src_file;
  char *namespace_name;

  char *buffer;
  u64 buffer_length;
  dynarray *tokens;
  int current_token_idx;

  ast_node_t *root;

  dynarray *errors;
} compilation_unit_t;

compilation_unit_t *new_compilation_unit(allocator_t, char *);
void compile(compilation_unit_t *);
