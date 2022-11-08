#pragma once

#include "str.h"

typedef struct str_builder_t {
  str *da_strs;
} str_builder_t;

str_builder_t *str_builder_init();
str_builder_t *str_builder_append_str(str_builder_t *, str);
str_builder_t *str_builder_append_i64(str_builder_t *, i64);
str_builder_t *str_builder_append_f64(str_builder_t *, f64);
str_builder_t *str_builder_append_char(str_builder_t *, char);
str_builder_t *str_builder_append_char_ptr(str_builder_t *, const char *);
str str_builder_to_alloced_str(str_builder_t *);
void str_builder_cleanup(str_builder_t *);

#define str_builder_append(sb, T)                                              \
  sb = _Generic((T),                                                           \
              str: str_builder_append_str,                                     \
              i64: str_builder_append_i64,                                     \
              f64: str_builder_append_f64,                                     \
             char: str_builder_append_char,                                    \
           char *: str_builder_append_char_ptr,                                \
     const char *: str_builder_append_char_ptr                                 \
)(sb, T);
