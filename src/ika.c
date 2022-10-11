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
#include "compiler.h"
#include "errors.h"
#include "ika.h"
#include "parser.h"
#include "print.h"
#include "tokenize.h"

int main(int argc, char **argv) {
  // Enable UTF-8 output for "fancy" line drawing..
  setlocale(LC_ALL, "C.UTF-8");

  // Create a simple linear allocator.  It will leak like
  // a sieve but we'll clean it all up in the end.
  allocator_t allocator =
      allocator_init(linear_allocator, (allocator_options){0});

  // Setup the logger
  log_init(allocator,
           (logger_configuration){.active_log_level = debug_log_level,
                                  .file_handle = stdout});

  if (argc != 2) {
    log_error("I just take a single source file for the moment.\n");
    exit(-1);
  }

  // Create a compilation unit around the source file, and compile it.
  compilation_unit_t *unit = new_compilation_unit(allocator, argv[1]);
  compile(unit);
}
