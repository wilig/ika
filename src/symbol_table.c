#include <stdlib.h>
#include <string.h>

#include "../lib/allocator.h"
#include "../lib/hashtbl.h"
#include "../lib/log.h"

#include "errors.h"
#include "symbol_table.h"
#include "types.h"

static uint32_t determine_byte_size(e_ika_type type) {
  switch (type) {
  case ika_int:
    return 8;
  case ika_float:
    return 8;
  default:
    return 8;
  }
}

symbol_table_t *make_symbol_table(symbol_table_t *parent) {
  symbol_table_t *symbol_table = imust_alloc(sizeof(symbol_table_t));
  symbol_table->parent = parent;

  // Build hashtable
  hashtbl_str_t *table = hashtbl_str_init();
  symbol_table->table = table;

  return symbol_table;
}

// Lookup the given symbol in the symbol table, if it's not found in the
// current scope, traverse back up the chain trying to resolve it.
symbol_table_entry_t *symbol_table_lookup(symbol_table_t *t, char *key) {
  str_entry_t *entry = hashtbl_str_lookup(t->table, cstr(key));
  if (entry) {
    return (symbol_table_entry_t *)entry->value;
  } else if (t->parent) { // Traverse up the chain trying to resolve
                          // the symbol
    printf("Traversing to parent\n");
    return symbol_table_lookup(t->parent, key);
  }
  return NULL;
}

IKA_STATUS symbol_table_insert(symbol_table_t *t, char *name, e_ika_type type,
                               bool constant, void *node_address,
                               uint32_t line) {
  symbol_table_entry_t *entry = imust_alloc(sizeof(symbol_table_entry_t));
  str *entry_key = imust_alloc(sizeof(str));
  str_copy(cstr(name), entry_key); // Leak
  entry->symbol = name;
  entry->bytes = determine_byte_size(type);
  entry->type = type;
  entry->constant = constant;
  entry->node_address = node_address;
  entry->line = line;
  if (!hashtbl_str_insert(
          t->table,
          (str_entry_t){.key = *entry_key, .valid = true, .value = entry})) {
    // The identifier is already in the symbol table.  That's an error.
    return ERROR_VARIABLE_REDEFINITION;
  }
  return SUCCESS;
}

void symbol_table_add_reference(symbol_table_t *t, char *key, uint32_t line,
                                uint32_t column) {
  symbol_table_entry_t *entry = symbol_table_lookup(t, key);
  if (entry) {
    symbol_table_reference_t *ref =
        imust_alloc(sizeof(symbol_table_reference_t));
    ref->column = column;
    ref->line = line;
  } else {
    ERROR("Undefined symbol %s referenced on line %d column %d\n", key, line,
          column);
  }
}

static void symbol_table_print_fill_space(u64 num_spaces) {
  for (u64 i = 0; i < num_spaces; i++) {
    printf(" ");
  }
}

static u64 count_digits(u64 n) {
  if (n == 0)
    return 1;
  u64 count = 0;
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
  for (u32 i = 0; i < ht_keys.count; i++) {
    symbol_table_entry_t *entry =
        symbol_table_lookup(t, str_to_cstr(*ht_keys.keys[i])); // LEAK
    printf("| %s", entry->symbol);
    symbol_table_print_fill_space(37 - strlen(entry->symbol));
    str type_name = cstr(ika_base_type_table[entry->type].label);
    printf("| %s", type_name.ptr);
    symbol_table_print_fill_space(19 - type_name.length);
    printf("| %i", entry->line);
    symbol_table_print_fill_space(6 - count_digits(entry->line));
    printf("| %p", entry->node_address);
    printf("|\n");
  }
  printf("--------------------------------------------------------------------"
         "----------\n\n");
}
