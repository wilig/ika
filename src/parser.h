#pragma once

#include <stdbool.h>
#include <sys/types.h>

#include "allocator.h"
#include "ast.h"
#include "hashtbl.h"
#include "tokenize.h"
#include "types.h"

/* typedef struct scope_t { */
/*   allocator_t allocator; */
/*   str name; */
/*   hashtbl_str_t symbol_table; */
/*   uint16_t level; */
/*   struct scope_t *parent; */
/* } scope_t; */

typedef struct {
  e_ika_type type;
  void *value;
  bool constant;
} symbol_table_entry_t;

typedef struct {
  allocator_t *allocator;
  str src_file;

  str *buffer;
  token **tokens;
  int current_token_idx;

  str *namespace_name;
  scope_t *scopes;
  scope_t *current_scope;
  import_stmt_t **import_statements;

  // TODO Add error arrays for warnings, and errors.
} compilation_unit_t;

void parser_parse(compilation_unit_t *);

void new_compilation_unit(allocator_t *, char *, str *, token **,
                          compilation_unit_t *);
