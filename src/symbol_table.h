#pragma once

#include "../lib/hashtbl.h"

#include "errors.h"
#include "types.h"

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
  bool constant;
  str *identifer;
  e_ika_type type;
  uint32_t bytes; // NOTE: bits might be better in the long run
  uint32_t alignment;
  int line;
  int column;
} symbol_table_entry_t;

typedef struct symbol_table_t {
  allocator_t allocator;
  struct symbol_table_t *parent;
  hashtbl_str_t *table;
} symbol_table_t;

symbol_table_t *make_symbol_table(allocator_t allocator,
                                  symbol_table_t *parent);

IKA_ERROR symbol_table_insert(symbol_table_t *, str name, e_ika_type type,
                              bool constant, uint32_t line, uint32_t column);

symbol_table_entry_t *symbol_table_lookup(symbol_table_t *, str);

void symbol_table_add_reference(symbol_table_t *, str, uint32_t line,
                                uint32_t column);

void symbol_table_dump(symbol_table_t *);
