#pragma once

#include "defines.h"
#include <stdio.h>

#define ANSI_ESCAPE_RED "\x1b[31m"
#define ANSI_ESCAPE_YELLOW "\x1b[33m"
#define ANSI_ESCAPE_GREEN "\x1b[32m"
#define ANSI_ESCAPE_DEFAULT "\x1b[0m"

typedef enum log_level {
  debug_log_level,
  info_log_level,
  warn_log_level,
  error_log_level,
  fatal_log_level,
} log_level;

typedef struct logger_configuration {
  enum log_level active_log_level;
  FILE *file_handle;
} logger_configuration;

void _do_log_entry(log_level, char *, ...);
void _log_fatal(char *file, u32 line, char *fmt, ...);

#define DEBUG(fmt, ...) _do_log_entry(debug_log_level, fmt, ##__VA_ARGS__)
#define INFO(fmt, ...) _do_log_entry(info_log_level, fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) _do_log_entry(warn_log_level, fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) _do_log_entry(error_log_level, fmt, ##__VA_ARGS__)
#define FATAL(fmt, ...) _log_fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

void initialize_logging(logger_configuration);

void shutdown_logging(void);
