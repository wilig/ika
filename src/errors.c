#include "errors.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int line_number;
  int offset_of_line;
  int column;
} SourcePosition;

void calcPosition(SourcePosition *sp, char *source, int offset) {
  sp->line_number = 1; // Human offset please.
  for (int i = 0; i <= offset; i++) {
    if (source[i] == '\n') {
      sp->line_number++;
      sp->offset_of_line = i + 1;
    }
  }
  sp->column = offset - sp->offset_of_line - 1;
}

void printSourceLine(char *source, int line_start_offset) {
  int i = line_start_offset + 1;
  while (source[i] != '\n')
    i++;
  printf("line_start_offset: %d, char: %d\n", i - line_start_offset + 1,
         source[line_start_offset]);
  printf("%.*s", i - line_start_offset + 1, &source[line_start_offset]);
}

void printAtColumn(char c, int column) {
  for (int i = 0; i <= column - 1; i++)
    printf(" ");
  printf("%c\n", c);
}

void printLineConnector(int column) {
  column -= 1;
  if (column > 0) {
    printf("|");
  }
  while (column > 0) {
    printf("-");
    column--;
  }
  printf("|\n");
  printf("|\n");
}

void printTrail(int column, int length) {
  while (length > 0) {
    printAtColumn('|', column);
    length--;
  }
  printLineConnector(column);
}

void displayCompilerError(char *source, int offset, char *msg, char *hint) {
  SourcePosition *sp = calloc(sizeof(SourcePosition), 1);
  calcPosition(sp, source, offset);
  printf("Compiler error:\n\n");
  printf("On line %d, column %d\n", sp->line_number, sp->column);
  if (source[offset] != 0) { // Not EOF so lets try to print a nice arrow
    printSourceLine(source, sp->offset_of_line);
    printAtColumn('^', sp->column);
    printTrail(sp->column, 4);
  }
  printf("%s\n", msg);
  if (hint) {
    printf("\nHint:\n\n%s\n", hint);
  }
  free(sp);
}
