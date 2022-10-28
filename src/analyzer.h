#pragma once

#include "compiler.h"

#include "errors.h"
#include "symbol_table.h"

typedef struct {
  syntax_error_t *errors;
  ast_node_t *parent;
  ast_node_t *current_function;
} analyzer_context_t;

void analyzer_analyze(compilation_unit_t *unit);
