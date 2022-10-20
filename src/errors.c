#include "errors.h"
#include <stdio.h>
#include <stdlib.h>

void errors_print_at_column(int c, int column) {
  for (int i = 0; i <= column - 1; i++)
    printf(" ");
  printf("%lc\n", c);
}

void errors_print_lines(int column) {
  column -= 1;
  if (column > 0) {
    printf("%lc", 0x250c);
  }
  while (column > 0) {
    printf("%lc", 0x2500);
    column--;
  }
  printf("%lc\n", 0x2518);
  printf("%lc\n", 0x2502);
}

void errors_print_pointer(int column, int length) {
  while (length > 0) {
    errors_print_at_column(0x25B2, column);
    length--;
  }
  errors_print_lines(column);
}

void errors_display_context(int line, str source) {
  int current_line = 0;
  int i = 0;
  do {
    char ch = str_get_char(source, i);
    if (current_line == line - 2 || current_line == line - 1 ||
        current_line == line) {
      printf("%c", ch);
    }
    if (ch == '\n')
      current_line += 1;

    i++;
  } while (current_line < line + 1 && i < source.length);
}

void errors_display_error(syntax_error_t *err, str source) {
  errors_display_context(err->line, source);
  errors_print_pointer(err->column, 1);
  printf("Stage: %d\n", err->pass);
  printf("Syntax Error: %.*s on line %d\n\n", err->message.length,
         err->message.ptr, err->line);
  if (err->hint.length > 0) {
    printf("%.*s\n\n", err->hint.length, err->hint.ptr);
  }
}

void errors_display_parser_errors(dynarray *errors, str source) {
  printf("%li errors during compilation\n", errors->count);
  for (int i = 0; i < errors->count; i++) {
    syntax_error_t *err = dynarray_get(errors, i);
    errors_display_error(err, source);
  }
}
