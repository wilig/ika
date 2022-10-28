#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "allocator.h"
#include "hashtbl.h"
#include "log.h"

static logger_configuration root_logger = (logger_configuration){
    .active_log_level = debug_log_level, .file_handle = NULL};

void _do_log_entry(log_level level, char *fmt, ...) {
  va_list args;
  FILE *out = root_logger.file_handle;
  switch (level) {
  case debug_log_level:
    fprintf(out, "DEBUG: ");
    break;
  case info_log_level:
    if (out == stdout)
      fprintf(out, ANSI_ESCAPE_GREEN);
    fprintf(out, "INFO: ");
    break;
  case warn_log_level:
    if (out == stdout)
      fprintf(out, ANSI_ESCAPE_YELLOW);
    fprintf(out, "WARN: ");
    break;
  case error_log_level:
    if (out == stdout)
      fprintf(out, ANSI_ESCAPE_RED);
    fprintf(out, "ERROR: ");
    break;
  case fatal_log_level:
    if (out == stdout)
      fprintf(out, ANSI_ESCAPE_RED);
    fprintf(out, "FATAL: ");
    break;
  }
  if (out == stdout)
    fprintf(out, ANSI_ESCAPE_DEFAULT);
  va_start(args, fmt);
  fprintf(out, fmt, args);
  va_end(args);
}

void _log_fatal(char *file, u32 line, char *format, ...) {
  va_list arg_pointer;
  va_start(arg_pointer, format);
  _do_log_entry(fatal_log_level, format, arg_pointer);
  va_end(arg_pointer);
  fprintf(root_logger.file_handle, "File: %s, line %d\n", file, line);
  exit(-1);
}

void initialize_logging(logger_configuration options) {
  root_logger.active_log_level = options.active_log_level;
  root_logger.file_handle = options.file_handle;
}

void shutdown_logging() { root_logger.file_handle = NULL; }
