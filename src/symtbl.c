#include "symtbl.h"
#include "allocator.h"
#include "hashtbl.h"
#include "log.h"
#include "parser.h"
#include "stdlib.h"
#include "types.h"

size_t determine_byte_size(e_ika_type type) {
  switch (type) {
  ika_bool:
  ika_u8:
  ika_i8 : { return 1; }
  ika_i16:
  ika_u16:
  ika_f16 : { return 2; }
  ika_i32:
  ika_u32:
  ika_f32 : { return 4; }
  ika_i64:
  ika_u64:
  ika_f64 : { return 8; }
  ika_rune : { return 4; }
  default: {
    return 8;
  }
  }
}

// TODO: Unify the allocation.  Pointers everywhere or values, NOT a mix.
symbol_table_t symbol_table_init(allocator_t allocator, scope_t *scope) {
  symbol_table_t *symtbl =
      allocator_alloc_or_exit(allocator, sizeof(symbol_table_t));
  symtbl->allocator = allocator;
  symtbl->scope = scope;

  // Build hashtable
  hashtbl_str_t table = hashtbl_str_init(allocator);
  symtbl->table = table;
  printf("symbol_table_init allocator points to: %p\n", &allocator);

  return *symtbl;
}

// Lookup the given symbol in the symbol table, if it's not found in the
// current scope, traverse back up the chain trying to resolve it.
symbol_table_entry_t *symbol_table_lookup(symbol_table_t table, str key) {
  str_entry_t entry = hashtbl_str_lookup(table.table, key);
  if (entry.valid) {
    return (symbol_table_entry_t *)entry.value;
  } else if (table.scope->parent) { // Traverse up the chain trying to resolve
                                    // the symbol
    return symbol_table_lookup(table.scope->parent->symbol_table, key);
  }
  return NULL;
}

void symbol_table_insert(symbol_table_t t, str key, e_ika_type type,
                         bool constant, size_t line, size_t column) {
  symbol_table_entry_t *entry =
      allocator_alloc_or_exit(t.allocator, sizeof(symbol_table_entry_t));
  entry->bytes = determine_byte_size(type);
  entry->type = type;
  entry->constant = constant;
  entry->line = line;
  entry->column = column;
  entry->reference_count = 0;
  if (!hashtbl_str_insert(t.table,
                          (str_entry_t){.key = key, .value = &entry})) {
    log_error("Redefinition of {s} at line {d} column {d}", key, line, column);
    exit(-1);
  };
}

void symbol_table_add_reference(symbol_table_t t, str key, size_t line,
                                size_t column) {
  symbol_table_entry_t *entry = symbol_table_lookup(t, key);
  if (entry) {
    symbol_table_reference_t *ref =
        allocator_alloc_or_exit(t.allocator, sizeof(symbol_table_reference_t));
    ref->column = column;
    ref->line = line;
    entry->references[entry->reference_count++] = ref;
  } else {
    log_error("Undefined symbol {s} referenced on line {d} column {d}", key,
              line, column);
  }
}
