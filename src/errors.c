#include "errors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void errors_print_at_column(int c, u32 column) {
  for (u32 i = 0; i <= column - 1; i++)
    printf(" ");
  printf("%lc\n", c);
}

static void errors_print_lines(u32 column) {
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

static void errors_print_pointer(u32 column, u32 length) {
  while (length > 0) {
    errors_print_at_column(0x25B2, column);
    length--;
  }
  errors_print_lines(column);
}

static void errors_display_context(u32 line, char *source) {
  u32 current_line = 0;
  u32 i = 0;
  do {
    char ch = source[i];
    if (current_line == line - 2 || current_line == line - 1 ||
        current_line == line) {
      printf("%c", ch);
    }
    if (ch == '\n')
      current_line += 1;

    i++;
  } while (current_line < line + 1 && i < strlen(source));
}

static void errors_display_error(syntax_error_t *err, char *source) {
  errors_display_context(err->line, source);
  errors_print_pointer(err->column, 1);
  printf("Stage: %d\n", err->pass);
  printf("Syntax Error: %s on line %d\n\n", err->message, err->line);
  if (err->hint) {
    printf("%s\n\n", err->hint);
  }
}

void errors_display_parser_errors(dynarray *errors, char *source) {
  printf("%li errors during compilation\n", errors->count);
  for (u32 i = 0; i < errors->count; i++) {
    syntax_error_t *err = dynarray_get(errors, i);
    errors_display_error(err, source);
  }
}
