#pragma once

#include "../lib/hashtbl.h"

#include "errors.h"
#include "tokens.h"
#include <stdint.h>

/* // Forward definition see parser.h */
typedef struct {
  uint32_t line;
  uint32_t column;
} symbol_table_reference_t;

// Basic symbol table entry.
//
// Basically maps human readable names to a information
// about a value.  Things like:
// - the name (possibly redundant)
// - a possibly preliminary type
// - size in bytes for the value
// - the alignment (it's a processor efficiency thing I think)
// - the location in the source it's defined
typedef struct {
  b8 constant;
  char *symbol;
  e_token_type type;
  u32 bytes;     // NOTE: bits might be better in the long run
  u32 dimension; // How many of type
  void *node_address;
  u32 line;
} symbol_table_entry_t;

typedef struct symbol_table_t {
  struct symbol_table_t *parent;
  hashtbl_str_t *table;
} symbol_table_t;

symbol_table_t *make_symbol_table(symbol_table_t *parent);

IKA_STATUS symbol_table_insert(symbol_table_t *, char *name, e_token_type type,
                               b8 constant, void *node, uint32_t line);

symbol_table_entry_t *symbol_table_lookup(symbol_table_t *, char *);

void symbol_table_add_reference(symbol_table_t *, char *, uint32_t line,
                                uint32_t column);

void symbol_table_dump(symbol_table_t *);
