#include "log.h"
#include "allocator.h"
#include "hashtbl.h"
#include "stdarg.h"
#include "stdio.h"
#include "tokenize.h"
#include <stdint.h>

static logger_configuration __LOGGER_CONFIG = {.valid = false};

hashtbl_str_t *get_type_lookup_table() { return __LOGGER_CONFIG.handlers; }

void output_str_impl(FILE *stream, str s) {
  if (stream != NULL) {
    for (uint32_t i = 0; i < s.length; i++) {
      fputc(s.ptr[i], stream);
    }
  }
}

void output_char_impl(FILE *stream, char *s) {
  output_str_impl(stream, cstr(s));
}

void _log_print_interpolated_str(FILE *stream, char *fmt, va_list args) {
  str format = cstr(fmt);
  if (str_contains(format, cstr("{"))) {
    uint32_t output_head = 0, end_brace = 0, count = 1;
    uint32_t start_brace = str_find_idx_of_nth(count, format, cstr("{"));
    while (start_brace != -1) {
      end_brace = str_find_idx_of_nth(count, format, cstr("}"));
      if (start_brace > 0 && end_brace != -1) {
        output(stream, cstr_from_char_with_length(&format.ptr[output_head],
                                                  start_brace - output_head));
      } else {
        // No closing brace so just dump the output
        output(stream, format);
        return;
      }
      output_head = end_brace + 1;
      str type_t =
          str_substr(format, start_brace + 1, (end_brace - start_brace) - 1);
      if (str_eq(type_t, cstr("s"))) {
        output(stream, va_arg(args, str));
      } else if (str_eq(type_t, cstr("c*"))) {
        output(stream, va_arg(args, char *));
      } else {
        hashtbl_str_t *type_lookup_table = get_type_lookup_table();
        str_entry_t *entry = hashtbl_str_lookup(type_lookup_table, type_t);
        if (entry) {
          void (*cb)(FILE *, void *) = (void (*)(FILE *, void *))entry->value;
          uint64_t value = va_arg(args, uint64_t);
          cb(stream, (void *)value);
        } else {
          output_str_impl(stderr, cstr("Unknown parameter type '"));
          output_str_impl(stderr, type_t);
          output_str_impl(stderr, cstr("' interpolation failed.\n"));
        }
        break;
      }
      // Next spot for interpolation
      start_brace = str_find_idx_of_nth(++count, format, cstr("{"));
    }
    if (output_head < format.length)
      output(stream, cstr_from_char_with_length(&format.ptr[output_head],
                                                format.length - output_head));
  } else {
    output(stream, format);
  }
}

log_level log_get_level() {
  if (__LOGGER_CONFIG.valid == false) {
    return 0xffffffff;
  } else {
    return __LOGGER_CONFIG.active_log_level;
  }
}

void log_set_color(FILE *stream, log_level level) {
  if (stream == stdout) {
    switch (level) {
    case debug_log_level:
      break;
    case info_log_level:
      output_str_impl(stream, ANSI_ESCAPE_GREEN);
      break;
    case warn_log_level:
      output_str_impl(stream, ANSI_ESCAPE_YELLOW);
      break;
    case error_log_level:
      output_str_impl(stream, ANSI_ESCAPE_RED);
      break;
    }
  }
}

void log_reset_color(FILE *stream) {
  if (stream == stdout) {
    output_str_impl(stream, ANSI_ESCAPE_DEFAULT);
  }
}

void print(char *fmt, ...) {
  va_list arg_pointer;
  va_start(arg_pointer, fmt);
  _log_print_interpolated_str(stdout, fmt, arg_pointer);
  va_end(arg_pointer);
}

void log_do_log_entry(logger_configuration logger, log_level level, char *fmt,
                      va_list args) {
  log_set_color(logger.file_handle, level);
  switch (level) {
  case debug_log_level:
    output(logger.file_handle, "DEBUG: ");
    break;
  case info_log_level:
    output(logger.file_handle, "INFO: ");
    break;
  case warn_log_level:
    output(logger.file_handle, "WARN: ");
    break;
  case error_log_level:
    output(logger.file_handle, "ERROR: ");
    break;
  }
  log_reset_color(logger.file_handle);
  _log_print_interpolated_str(logger.file_handle, fmt, args);
}

void log_debug(char *format, ...) {
  va_list arg_pointer;
  if (log_get_level() <= debug_log_level) {
    va_start(arg_pointer, format);
    log_do_log_entry(__LOGGER_CONFIG, debug_log_level, format, arg_pointer);
    va_end(arg_pointer);
  }
}

void log_info(char *format, ...) {
  va_list arg_pointer;
  if (log_get_level() <= info_log_level) {
    va_start(arg_pointer, format);
    log_do_log_entry(__LOGGER_CONFIG, info_log_level, format, arg_pointer);
    va_end(arg_pointer);
  }
}

void log_warn(char *format, ...) {
  va_list arg_pointer;
  if (log_get_level() <= warn_log_level) {
    va_start(arg_pointer, format);
    log_do_log_entry(__LOGGER_CONFIG, warn_log_level, format, arg_pointer);
    va_end(arg_pointer);
  }
}

void log_error(char *format, ...) {
  va_list arg_pointer;
  if (log_get_level() <= error_log_level) {
    va_start(arg_pointer, format);
    log_do_log_entry(__LOGGER_CONFIG, error_log_level, format, arg_pointer);
    va_end(arg_pointer);
  }
}

void log_init(allocator_t allocator, logger_configuration options) {
  __LOGGER_CONFIG.file_handle = options.file_handle;
  __LOGGER_CONFIG.active_log_level = options.active_log_level;
  __LOGGER_CONFIG.handlers = hashtbl_str_init(allocator);
  __LOGGER_CONFIG.valid = true;
}

void log_deinit() {
  // hashtbl_str_deinit(get_type_lookup_table());
}

void log_register_type(str key, void func(FILE *, void *)) {
  log_info("registering a logging type for '{s}'\n", key);
  str_entry_t i =
      (str_entry_t){.key = key, .value = (void *)func, .valid = true};
  hashtbl_str_t *table = get_type_lookup_table();
  hashtbl_str_insert(table, i);
}
