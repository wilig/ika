#pragma once

#include "hashtbl.h"
#include "tokenize.h"
#include <stdio.h>

#define ANSI_ESCAPE_RED cstr("\x1b[31m")
#define ANSI_ESCAPE_YELLOW cstr("\x1b[33m")
#define ANSI_ESCAPE_GREEN cstr("\x1b[32m")
#define ANSI_ESCAPE_DEFAULT cstr("\x1b[0m")

typedef enum log_level {
  debug_log_level,
  info_log_level,
  warn_log_level,
  error_log_level
} log_level;

typedef struct logger_configuration {
  bool valid;
  enum log_level active_log_level;
  FILE *file_handle;
  hashtbl_str_t handlers;
} logger_configuration;

void log_debug(char *, ...);
void log_info(char *, ...);
void log_warn(char *, ...);
void log_error(char *, ...);

void log_init(allocator_t, logger_configuration);
void log_deinit();
void log_register_type(str, void func(FILE *, void *));

#define output(fh, value)                                                      \
  _Generic((value), char * : output_char_impl, default : output_str_impl)(fh, value)

/* // The ## before __VA_ARGS__ swallows the comman to allow an empty list. */
/* #define log_debug(logger_t, fmt, ...) \ */
/*   log_debug_impl(logger_t, maybe_convert_to_str(fmt), ##__VA_ARGS__) */

/* #define log_info(logger_t, fmt, ...) \ */
/*   log_info_impl(logger_t, maybe_convert_to_str(fmt), ##__VA_ARGS__) */

/* #define log_warn(logger_t, fmt, ...) \ */
/*   log_warn_impl(logger_t, maybe_convert_to_str(fmt), ##__VA_ARGS__) */

/* #define log_error(logger_t, fmt, ...) \ */
/*   log_error_impl(logger_t, maybe_convert_to_str(fmt), ##__VA_ARGS__) */