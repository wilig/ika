#include "../lib/assert.h"
#include "../lib/format.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "errors.h"
#include "tokenize.h"

static void errors_print_at_column(int c, u32 column) {
  for (u32 i = 0; i < column; i++)
    printf(" ");
  printf("%lc\n", c);
}

// TODO: This is a mess.  Fix it.
static void errors_print_lines(u32 column) {
  u32 width = column;
  if (column > 0) {
    printf("%lc", 0x250c);
  }
  if (width > 0) {
    while (width - 1 > 0) {
      printf("%lc", 0x2500);
      width--;
    }
  }
  if (column > 0) {
    printf("%lc\n", 0x2518);
  } else {
    printf("%lc\n", 0x2502);
  }
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
  printf("Syntax Error: line %d, column %d\n\n", err->line, err->column);
  printf("%s\n", err->message);
}

void errors_display_parser_errors(da_syntax_errors *errors, char *source) {
  printf("%li errors during compilation\n", darray_len(errors));
  for (u32 i = 0; i < darray_len(errors); i++) {
    errors_display_error(&errors[i], source);
  }
}
