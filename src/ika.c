#include <locale.h>

#include "../lib/allocator.h"
#include "../lib/log.h"

#include "compiler.h"
#include "ika.h"

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
