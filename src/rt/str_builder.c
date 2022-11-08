
#include "../../lib/allocator.h"

#include "darray.h"
#include "str.h"
#include "str_builder.h"
#include <stdio.h>
#include <string.h>

str_builder_t *str_builder_init() {
  str_builder_t *builder = imust_alloc(sizeof(str_builder_t));
  builder->da_strs = darray_init(str);
  return builder;
}

str_builder_t *str_builder_append_str(str_builder_t *sb, str s) {
  darray_append(sb->da_strs, s);
  return sb; // TODO: Update darray append to return bool
}

str_builder_t *str_builder_append_char_ptr(str_builder_t *sb, const char *cp) {
  return str_builder_append_str(sb, cstr(cp));
}

str_builder_t *str_builder_append_char(str_builder_t *sb, const char ch) {
  char *buff = imust_alloc(2);
  snprintf(buff, 2, "%c", ch);
  return str_builder_append_str(sb, cstr(buff));
}

str_builder_t *str_builder_append_i64(str_builder_t *sb, i64 n) {
  char *buff = imust_alloc(20 + 1);
  snprintf(buff, 21, "%li", n);
  return str_builder_append_str(sb, cstr(buff));
}

str_builder_t *str_builder_append_f64(str_builder_t *sb, f64 n) {
  // https://stackoverflow.com/questions/56514892/how-many-digits-can-float8-float16-float32-float64-and-float128-contain
  char *buff = imust_alloc(15 + 1);
  snprintf(buff, 16, "%lf", n);
  return str_builder_append_str(sb, cstr(buff));
}

str str_builder_to_alloced_str(str_builder_t *sb) {
  u64 total_length = 0;
  for (int i = 0; i < darray_len(sb->da_strs); i++) {
    total_length += sb->da_strs[i].length;
  }
  char *buffer = imust_alloc(total_length);
  char *copy_pointer = buffer;
  for (int i = 0; i < darray_len(sb->da_strs); i++) {
    memcpy(copy_pointer, sb->da_strs[i].ptr, sb->da_strs[i].length);
    copy_pointer += sb->da_strs[i].length;
  }
  return (str){.ptr = (const char *)buffer, .length = total_length};
}

void str_builder_cleanup(str_builder_t *sb) { darray_deinit(sb->da_strs); }
