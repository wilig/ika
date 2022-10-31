#include <locale.h>
#include <sys/stat.h>

#include "../lib/allocator.h"
#include "../lib/log.h"

#include "compiler.h"
#include "helpers.h"
#include "ika.h"

typedef struct cmdargs {
  b8 is_valid;
  char *src_file;
  u64 src_len;
  b8 verbose;
} cmdargs;

static void print_help() {
  printf("Usage: ika [-v] source_file\n\n");
  printf("-v  Output verbose information about the compilation process.\n");
  printf("-h  Print this help message.\n");
  printf("source_file  A source file to compile.\n\n");
}

static cmdargs parse_args(int argc, char **argv) {
  cmdargs args = {.is_valid = FALSE, .verbose = FALSE};
  struct stat info;
  if (argc > 3 || argc <= 1)
    return args;
  for (int i = 0; i < argc; i++) {
    if (argv[i][0] == '-') { // it's a flag
      if (streq(argv[i], "-h")) {
        return args;
      } else if (streq(argv[i], "-v")) {
        args.verbose = TRUE;
      } else {
        printf("Unknown flag %s\n\n", argv[i]);
        return args;
      }
    } else {
      if (stat(argv[i], &info) < 0) {
        perror("Unable to open file: ");
        exit(-2);
      } else {
        args.src_file = argv[i];
        args.src_len = (u64)info.st_size;
      }
    }
  }
  args.is_valid = TRUE;
  return args;
}

int main(int argc, char **argv) {
  // Enable UTF-8 output for "fancy" line drawing..
  setlocale(LC_ALL, "C.UTF-8");

  if (!initialize_allocator()) {
    FATAL("Couldn't initialize allocator");
  }

  // Setup the logger
  initialize_logging((logger_configuration){.active_log_level = debug_log_level,
                                            .file_handle = stdout});

  cmdargs args = parse_args(argc, argv);
  if (args.is_valid) {
    // Create a compilation unit around the source file, and compile it.
    compilation_unit_t *unit =
        new_compilation_unit(args.src_file, args.src_len, args.verbose);
    compile(unit);
  } else {
    print_help();
  }
}
