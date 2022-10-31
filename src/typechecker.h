#pragma once

#include "compiler.h"

#include "errors.h"
#include "symbol_table.h"

typedef struct {
  syntax_error_t *errors;
  ast_node_t *parent;
  ast_node_t *current_function;
} tc_context_t;

void tc_check(compilation_unit_t *unit);
