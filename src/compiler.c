#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include "rt/darray.h"

#include "../lib/log.h"

#include "backend/c11.h"
#include "compiler.h"
#include "errors.h"
#include "parser.h"
#include "print.h"
#include "symbol_table.h"
#include "tokenize.h"
#include "typechecker.h"

typedef struct timings {
  u64 tokenization;
  u64 parsing;
  u64 analyzation;
} timings;

static u64 time_in_ms() {
  struct timespec now;
  timespec_get(&now, TIME_UTC);
  return ((u64)now.tv_sec) * 1000 + ((u64)now.tv_nsec) / 1000000;
}

compilation_unit_t *new_compilation_unit(char *filename, u64 file_length,
                                         b8 verbose) {
  // Allocate all the memory for loading the file, add a byte to the end
  // for the trailing zero.
  char *buffer = imust_alloc(file_length + 1);
  FILE *fh = fopen(filename, "r");
  if (fh == NULL) {
    perror("Failed to open file: ");
    exit(-1);
  }

  INFO("Just about to read file into memory\n");
  // Read in the whole file
  u64 read = fread(buffer, sizeof(uint8_t), file_length, fh);
  if (read != file_length) {
    ERROR("Failed to read complete file, expected %li bytes, read %li "
          "bytes.\nBailing out.\n",
          file_length, read);
  }

  compilation_unit_t *unit = imust_alloc(sizeof(compilation_unit_t));
  unit->src_file = filename;
  unit->verbose = verbose;
  unit->buffer = buffer;
  unit->buffer_length = read;
  unit->current_token_idx = 0;
  unit->errors = darray_init(syntax_error_t);
  return unit;
}

void compile(compilation_unit_t *unit) {
  timings timer = {0};
  u64 start = time_in_ms();
  if (unit->verbose)
    printf("\n-------------------------------------\nTokenization pass\n");
  da_tokens *tokens =
      tokenizer_scan(unit->buffer, unit->buffer_length, unit->errors);
  timer.tokenization = time_in_ms() - start;

  if (unit->verbose) {
    /* Print tokens */
    for (u64 i = 0; i < darray_len(tokens); i++) {
      tokenizer_print_token(stdout, &tokens[i]);
      printf("\n");
    }
  }

  start = time_in_ms();
  if (unit->verbose)
    printf("\n-------------------------------------\nParser pass\n");
  ast_node_t *root = parser_parse(tokens, unit->errors);
  timer.parsing = time_in_ms() - start;
  unit->root = root;

  start = time_in_ms();
  if (unit->verbose)
    printf("\n-------------------------------------\nTyping pass\n");
  tc_check(unit);
  timer.analyzation = time_in_ms() - start;

  if (darray_len(unit->errors) > 0) {
    errors_display_parser_errors(unit->errors, unit->buffer);
  } else {
    if (unit->verbose) {
      print_node_as_tree(root, 0);
      printf("Root Symbol Table:\n");
      print_symbol_table(root->block.symbol_table);
    }
    unit->root = root;
    c11_generate(unit);
    printf("\nTimings:\n");
    printf("Tokenization took: %li ms\n", timer.tokenization);
    printf("Parsing took: %li ms\n", timer.parsing);
    printf("Analyzation took: %li ms\n", timer.analyzation);
    printf("Compilation complete for: %s\n", unit->src_file);
  }
}
