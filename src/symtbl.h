#pragma once

#include "../lib/hashtbl.h"
#include "types.h"

// Forward definition see parser.h
typedef struct scope_t scope_t;

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
// - potentially a memory location
// - size in bytes for the value
// - the alignment (it's a processor efficiency thing I think)
// - the location in the source it's defined
// - other locations that reference it
typedef struct {
  str *name;
  e_ika_type type;
  void *storage;
  uint32_t bytes; // NOTE: bits might be better in the long run
  uint32_t alignment;
  uint32_t line;
  uint32_t column;
  bool constant;
  symbol_table_reference_t **references;
  uint32_t reference_count;
} symtbl_entry_t;

typedef struct {
  allocator_t allocator;
  scope_t *scope;
  hashtbl_str_t *table;
} symtbl_t;

symtbl_t *symtbl_init(allocator_t allocator, scope_t *scope);

void symtbl_insert(symtbl_t *, str name, e_ika_type type, bool constant,
                   uint32_t line, uint32_t column);

symtbl_entry_t *symtbl_lookup(symtbl_t *, str);

void symtbl_add_reference(symtbl_t *, str, uint32_t line, uint32_t column);

void symtbl_dump(symtbl_t *);
