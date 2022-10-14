#include <stdlib.h>

#include "../lib/allocator.h"
#include "../lib/hashtbl.h"
#include "../lib/log.h"

#include "errors.h"
#include "symbol_table.h"
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

symbol_table_t *make_symbol_table(allocator_t allocator,
                                  symbol_table_t *parent) {
  symbol_table_t *symbol_table =
      allocator_alloc_or_exit(allocator, sizeof(symbol_table_t));
  symbol_table->allocator = allocator;
  symbol_table->parent = parent;

  // Build hashtable
  hashtbl_str_t *table = hashtbl_str_init(allocator);
  symbol_table->table = table;

  return symbol_table;
}

// Lookup the given symbol in the symbol table, if it's not found in the
// current scope, traverse back up the chain trying to resolve it.
symbol_table_entry_t *symbol_table_lookup(symbol_table_t *t, str key) {
  str_entry_t *entry = hashtbl_str_lookup(t->table, key);
  if (entry) {
    return (symbol_table_entry_t *)entry->value;
  } else if (t->parent) { // Traverse up the chain trying to resolve
                          // the symbol
    printf("Traversing to parent\n");
    return symbol_table_lookup(t->parent, key);
  }
  return NULL;
}

IKA_ERROR symbol_table_insert(symbol_table_t *t, str name, e_ika_type type,
                              bool constant, uint32_t line, uint32_t column) {
  symbol_table_entry_t *entry =
      allocator_alloc_or_exit(t->allocator, sizeof(symbol_table_entry_t));
  str *entry_name = allocator_alloc_or_exit(t->allocator, sizeof(str));
  str_copy(t->allocator, name, entry_name);
  entry->identifer = entry_name;
  entry->bytes = determine_byte_size(type);
  entry->type = type;
  entry->constant = constant;
  entry->line = line;
  entry->column = column;
  if (!hashtbl_str_insert(
          t->table,
          (str_entry_t){.key = name, .valid = true, .value = entry})) {
    // The identifier is already in the symbol table.  That's an error.
    return VARIABLE_REDEFINITION_ERROR;
  };
  return SUCCESS;
}

void symbol_table_add_reference(symbol_table_t *t, str key, uint32_t line,
                                uint32_t column) {
  symbol_table_entry_t *entry = symbol_table_lookup(t, key);
  if (entry) {
    symbol_table_reference_t *ref =
        allocator_alloc_or_exit(t->allocator, sizeof(symbol_table_reference_t));
    ref->column = column;
    ref->line = line;
  } else {
    log_error("Undefined symbol {s} referenced on line {d} column {d}", key,
              line, column);
  }
}

static void symbol_table_print_fill_space(int num_spaces) {
  for (int i = 0; i < num_spaces; i++) {
    printf(" ");
  }
}

int count_digits(size_t n) {
  if (n == 0)
    return 1;
  int count = 0;
  while (n != 0) {
    n = n / 10;
    count++;
  }
  return count;
}

void symbol_table_dump(symbol_table_t *t) {
  printf("--------------------------------------------------------------------"
         "----------\n");
  printf("| Name                                 | Type               | Line "
         "| Column |\n");
  printf("|--------------------------------------|--------------------|-------"
         "|--------|\n");

  hashtbl_str_keys_t ht_keys = hashtbl_str_get_keys(t->table);
  for (int i = 0; i < ht_keys.count; i++) {
    symbol_table_entry_t *entry = symbol_table_lookup(t, *ht_keys.keys[i]);
    printf("| %.*s", entry->identifer->length, entry->identifer->ptr);
    symbol_table_print_fill_space(37 - entry->identifer->length);
    str type_name = cstr(ika_base_type_table[entry->type].label);
    printf("| %s", type_name.ptr);
    symbol_table_print_fill_space(19 - type_name.length);
    printf("| %i", entry->line);
    symbol_table_print_fill_space(6 - count_digits(entry->line));
    printf("| %i", entry->column);
    symbol_table_print_fill_space(7 - count_digits(entry->column));
    printf("|\n");
  }
  printf("--------------------------------------------------------------------"
         "----------\n\n");
}
