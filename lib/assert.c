#include "assert.h"

#include <stdint.h>
#include <stdio.h>

void report_assertion_failure(const char *expression, const char *message,
                              const char *file, int32_t line) {
  printf("Assertion Failure: %s, message: '%s', in file: %s, line: %d\n",
         expression, message, file, line);
}
