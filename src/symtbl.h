#pragma once

#include "hashtbl.h"

// Forward definition see parser.h
typedef struct scope_t scope_t;

typedef struct {
  size_t line;
  size_t column;
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
  size_t bytes; // NOTE: bits might be better in the long run
  size_t alignment;
  size_t line;
  size_t column;
  symbol_table_reference_t **references;
  size_t reference_count;
} symbol_table_entry_t;

typedef struct {
  allocator_t *allocator;
  scope_t *scope;
  hashtbl_str_t *table;
} symbol_table_t;

symbol_table_t *symbol_table_init(allocator_t allocator);

void symbol_table_insert(symbol_table_t *, str, e_ika_type, size_t, size_t);

symbol_table_entry_t *symbol_table_lookup(symbol_table_t *, str);

void symbol_table_add_reference(symbol_table_t *, str, size_t line,
                                size_t column);

/* typedef struct { */
/*   e_ika_type type_of_number; */
/*   union { */
/*     uint8_t uint8; */
/*     uint16_t uint16; */
/*     uint32_t uint32; */
/*     uint64_t uinut64; */
/*     int8_t int8; */
/*     int16_t int16; */
/*     int32_t int32; */
/*     int64_t int64; */
/*     float flt16; */
/*     long flt32; */
/*     long long flt64; */
/*   }; */
/* } ika_numeric_value_t; */

/* typedef struct { */
/*   uint8_t rune[4]; */
/* } ika_rune_value_t; */

/* typedef struct { */
/*   uint8_t *raw_bytes; */
/*   uint64_t length; */
/* } ika_string_value_t; */

/* typedef struct { */
/*   e_ika_type type; */
/*   union { */
/*     ika_numeric_value_t number; */
/*     ika_rune_value_t rune; */
/*     ika_string_value_t string; */
/*   }; */
/* } ika_typed_literal_t; */

/* typedef struct { */
/*   e_ika_type return_type; */
/*   e_ika_type *parameters; */
/*   size_t parameter_count; */
/* } ika_typed_function_definition_t; */

/* typedef struct { */
/*   e_ika_type type; */
/*   str *identifier; */
/* } ika_typed_identifier_t; */

/* typedef struct { */
/*   e_ika_type type; */
/*   expr_t *expr; */
/* } ika_typed_expression_t; */

/* typedef enum { */
/*   ika_typed_literal, */
/*   ika_typed_identifier, */
/*   ika_typed_function_definition, */
/*   ika_typed_expression, */
/* } ika_typed_value; */

/* typedef struct { */
/*   ika_typed_value type; */
/*   union { */
/*     ika_typed_literal_t literal; */
/*     ika_typed_function_definition_t function; */
/*     ika_typed_expression_t expression; */
/*     ika_typed_identifier_t identifier; */
/*   }; */
/* } symbol_table_entry_t; */
