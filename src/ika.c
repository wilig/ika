#include <assert.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "../lib/allocator.h"
#include "../lib/dynarray.h"
#include "../lib/hashtbl.h"
#include "../lib/log.h"

#include "analyzer.h"
#include "ika.h"
#include "parser.h"
#include "print.h"
#include "tokenize.h"

int main(int argc, char **argv) {
  // Enable UTF-8 output for "fancy" line drawing..
  setlocale(LC_ALL, "C.UTF-8");
  allocator_t allocator =
      allocator_init(linear_allocator, (allocator_options){0});
  log_init(allocator,
           (logger_configuration){.active_log_level = debug_log_level,
                                  .file_handle = stdout});
  log_register_type(cstr("token"), tokenizer_print_token);
  struct stat info;
  // char *buffer;
  if (argc != 2) {
    log_error("I just take a single source file for the moment.\n");
    exit(-1);
  }
  if (stat(argv[1], &info) < 0) {
    perror("Failed to open file: ");
    exit(-2);
  }
  if (info.st_size > 100000) {
    log_error(
        "Um, that's a bit too big of a file for little ol' me.  Lower your "
        "expectations please.");
    exit(-3);
  }
  log_info("Just about to allocate memory\n");
  allocated_memory mem =
      allocator_alloc(allocator, sizeof(uint8_t) * info.st_size);
  if (mem.valid != true) {
    log_error("Failed to allocate memory to load the file contents\n");
    exit(-1);
  }
  log_info("Just about to fopen file\n");
  FILE *fh = fopen(argv[1], "r");
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
  str contents = {.ptr = mem.ptr, .length = read};
  tokenizer_input_stream stream = {.source = contents, .allocator = allocator};
  dynarray *tokens = tokenizer_scan(&stream);

  for (int x = 0; x < tokens->count; x++) {
    log_info("Found {token}\n", dynarray_get(tokens, x));
  }
  compilation_unit_t unit = (compilation_unit_t){.src_file = {argv[1]},
                                                 .allocator = allocator,
                                                 .buffer = &contents,
                                                 .tokens = tokens};
  ast_node_t *root = parser_parse(&unit);
  printf("ROOT\n");
  print_node_as_tree(root, 0);
}
