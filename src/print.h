#pragma once

#include "parser.h"
#include "symbol_table.h"

void print_node_as_sexpr(ast_node_t *);
void print_node_as_tree(ast_node_t *, uint32_t);
void print_symbol_table(symbol_table_t *);
