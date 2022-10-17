#pragma once

#include "compiler.h"

#include "../lib/dynarray.h"
#include "symbol_table.h"

typedef struct {
  dynarray *errors;
  symbol_table_t *symbol_table;
} analyzer_context_t;

void analyzer_analyze(compilation_unit_t *unit);
