#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include "../lib/darray.h"
#include "../lib/log.h"

#include "analyzer.h"
#include "compiler.h"
#include "errors.h"
#include "parser.h"
#include "print.h"
#include "symbol_table.h"
#include "tokenize.h"

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

compilation_unit_t *new_compilation_unit(allocator_t allocator,
                                         char *filename) {
  struct stat info;
  if (stat(filename, &info) < 0) {
    perror("Failed to open file: ");
    exit(-2);
  }
  if (info.st_size > 100000) {
    log_error(
        "Source file too large.  Let's keep it simple for the time being.");
    exit(-3);
  }
  // Allocate all the memory for loading the file, add a byte to the end
  // for the trailing zero.
  char *buffer = allocator_alloc_or_exit(allocator, (u64)info.st_size + 1);
  FILE *fh = fopen(filename, "r");
  if (fh == NULL) {
    perror("Failed to open file: ");
    exit(-1);
  }

  log_info("Just about to read file into memory\n");
  // Read in the whole file
  u64 read = fread(buffer, sizeof(uint8_t), (u32)info.st_size, fh);
  if (read != (u64)info.st_size) {
    log_error("Failed to read complete file, expected {l} bytes, read {l} "
              "bytes.\nBailing out.\n",
              info.st_size, read);
  }

  compilation_unit_t *unit =
      allocator_alloc_or_exit(allocator, sizeof(compilation_unit_t));
  unit->allocator = allocator;
  unit->buffer = buffer;
  unit->buffer_length = read;
  unit->current_token_idx = 0;
  unit->errors = darray_init(allocator, syntax_error_t);
  return unit;
}

void compile(compilation_unit_t *unit) {
  timings timer = {0};
  u64 start = time_in_ms();
  printf("\n-------------------------------------\nTokenization pass\n");
  da_tokens *tokens = tokenizer_scan(unit->allocator, unit->buffer,
                                     unit->buffer_length, unit->errors);
  timer.tokenization = time_in_ms() - start;

  /* Print tokens */
  for (u64 i = 0; i < darray_len(tokens); i++) {
    tokenizer_print_token(stdout, &tokens[i]);
    printf("\n");
  }

  start = time_in_ms();
  printf("\n-------------------------------------\nParser pass\n");
  ast_node_t *root = parser_parse(unit->allocator, tokens, unit->errors);
  timer.parsing = time_in_ms() - start;
  unit->root = root;

  start = time_in_ms();
  printf("\n-------------------------------------\nAnalyzer pass\n");
  analyzer_analyze(unit);
  timer.analyzation = time_in_ms() - start;
  printf("\nDone.\n");

  if (darray_len(unit->errors) > 0) {
    errors_display_parser_errors(unit->errors, unit->buffer);
  } else {
    print_node_as_tree(root, 0);
    printf("Root Symbol Table:\n");
    symbol_table_dump(root->block.symbol_table);
    printf("\nTimings:\n");
    printf("Tokenization took: %li ms\n", timer.tokenization);
    printf("Parsing took: %li ms\n", timer.parsing);
    printf("Analyzation took: %li ms\n", timer.analyzation);
  }
}
