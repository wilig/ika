#include <stdio.h>
#include <sys/stat.h>

#include "../lib/log.h"

#include "compiler.h"
#include "errors.h"
#include "parser.h"
#include "print.h"
#include "tokenize.h"

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
  allocated_memory mem =
      allocator_alloc(allocator, sizeof(uint8_t) * info.st_size);
  if (mem.valid != true) {
    log_error("Failed to allocate memory to load the file contents\n");
    exit(-1);
  }
  FILE *fh = fopen(filename, "r");
  if (fh == NULL) {
    perror("Failed to open file: ");
    exit(-1);
  }

  log_info("Just about to read file into memory\n");
  // Read in the whole file
  long read = fread(mem.ptr, sizeof(uint8_t), info.st_size, fh);
  if (read != info.st_size) {
    log_error("Failed to read complete file, expected {l} bytes, read {l} "
              "bytes.\nBailing out.\n",
              info.st_size, read);
  }

  // Allocate a str for wrapping the file contents
  str *buffer = allocator_alloc_or_exit(allocator, sizeof(str));
  buffer->ptr = mem.ptr;
  buffer->length = read;

  compilation_unit_t *unit =
      allocator_alloc_or_exit(allocator, sizeof(compilation_unit_t));
  unit->allocator = allocator;
  unit->buffer = buffer;
  unit->current_token_idx = 0;
  unit->errors = dynarray_init(allocator, sizeof(syntax_error_t));

  return unit;
}

void compile(compilation_unit_t *unit) {
  dynarray *errors = dynarray_init(unit->allocator, sizeof(syntax_error_t));

  dynarray *tokens = tokenizer_scan(unit->allocator, *unit->buffer, errors);
  printf("TOKENS:\n");
  for (int i = 0; i < tokens->count; i++) {
    tokenizer_print_token(stdout, dynarray_get(tokens, i));
    printf("\n");
  }

  ast_node_t *root = parser_parse(unit->allocator, tokens, errors);

  if (errors->count > 0) {
    errors_display_parser_errors(errors, *unit->buffer);
  } else {
    print_node_as_tree(root, 0);
  }
}