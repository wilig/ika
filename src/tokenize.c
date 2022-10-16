#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/log.h"
#include "../lib/str.h"

#include "defines.h"
#include "errors.h"
#include "tokenize.h"
#include "types.h"

static token_position_t tokenizer_calculate_position(str source,
                                                     uint32_t offset) {
  int line = 0;
  int column = 0;
  for (int x = 0; x < offset; x++) {
    if (str_get_char(source, x) == '\n') {
      line++;
      column = 0;
    } else {
      column++;
    }
  }
  return (token_position_t){.column = column, .line = line};
}

static int is_newline(tokenizer_input_stream *s) {
  char c = str_get_char(s->source, s->pos);
  return (c == '\n');
}

static bool is_alpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ? true
                                                                      : false;
}

static bool is_alpha_numeric(char c) { // Couldn't resist X)
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ||
                 c >= '0' && c <= '9'
             ? true
             : false;
}

static char current_char(tokenizer_input_stream *s) {
  return str_get_char(s->source, s->pos);
}

static bool is_string_marker(tokenizer_input_stream *s) {
  char c = str_get_char(s->source, s->pos);
  return c == '"' ? true : false;
}

static bool is_comment(tokenizer_input_stream *s) {
  return str_matches_at_index(s->source, cstr("//"), s->pos) ||
         str_matches_at_index(s->source, cstr("/*"), s->pos);
}

static bool begins_multiline_comment(tokenizer_input_stream *s) {
  return str_matches_at_index(s->source, cstr("/*"), s->pos);
}

static bool ends_multiline_comment(tokenizer_input_stream *s) {
  return str_matches_at_index(s->source, cstr("*/"), s->pos);
}

static bool is_digit_marker(tokenizer_input_stream *s) {
  char c = current_char(s);
  return c >= '0' && c <= '9' ? true : false;
}

static bool is_numeric(tokenizer_input_stream *s) {
  char c = current_char(s);
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
                 (c >= 'A' && c <= 'F') || c == 'x' || c == 'o' || c == '.' ||
                 c == '-'
             ? true
             : false;
}

static bool is_valid_string_escape_character(tokenizer_input_stream *s) {
  char c = current_char(s);
  return c == '\\' || c == 'n' || c == 't' || c == '"' ? true : false;
}

static str tokenizer_extract_value(tokenizer_input_stream *s, uint32_t start,
                                   uint32_t end) {
  return str_substr_copy(s->allocator, s->source, start, end - start);
}

static bool is_operator(tokenizer_input_stream *s) {
  char ch = current_char(s);
  for (int i = __ika_operators_start + 1; i < __ika_operators_end - 1; i++) {
    if (ch == ika_base_type_table[i].txt[0]) {
      return true;
    }
  }
  return false;
}

static token_t tokenize_string(tokenizer_input_stream *s) {
  assert(is_string_marker(s));
  int starting_offset = ++s->pos; // Skip the quote
  bool escaped = false;
  while ((!is_string_marker(s) || escaped) && s->pos <= s->source.length) {
    // Handle invalid escape codes
    if (escaped && !is_valid_string_escape_character(s)) { // Handle error
      token_position_t pos = tokenizer_calculate_position(s->source, s->pos);
      syntax_error_t err = {.line = pos.line,
                            .column = pos.column,
                            .pass = tokenizing_pass,
                            .message = "Invalid string escape sequence.",
                            .hint = "Valid sequences are \\\\ \\n \\t "
                                    "\\\"."};
      dynarray_append(s->errors, &err);
    } else {
      escaped = (!escaped && current_char(s) == '\\');
    }
    s->pos += 1;
  }
  if (!is_string_marker(s)) {
    token_position_t pos = tokenizer_calculate_position(s->source, s->pos);
    syntax_error_t err = {
        .line = pos.line,
        .column = pos.column,
        .pass = tokenizing_pass,
        .message = "Reached the end of the file before finding a string "
                   "termination character.",
        .hint =
            "String should be terminated with a \" at the end of the string"};
    dynarray_append(s->errors, &err);
  }
  return (token_t){
      .type = ika_str_literal,
      .value = tokenizer_extract_value(s, starting_offset, s->pos++),
      .position = tokenizer_calculate_position(s->source, starting_offset - 1)};
}

static token_t tokenize_comment(tokenizer_input_stream *s) {
  assert(is_comment(s));
  int starting_offset = s->pos;
  bool multiline = begins_multiline_comment(s);
  int comment_depth = 1;
  s->pos += 2;
  while (comment_depth > 0 && s->pos <= s->source.length) {
    if (!multiline) {
      if (is_newline(s)) {
        comment_depth -= 1;
        continue;
      }
    } else {
      if (begins_multiline_comment(s)) {
        comment_depth += 1;
      } else if (ends_multiline_comment(s)) {
        comment_depth -= 1;
      }
    }
    s->pos++;
  }
  // Check for unterminated comment.  Depth > 0 means we reached EOF.
  if (comment_depth != 0) {
    token_position_t pos = tokenizer_calculate_position(s->source, s->pos);
    syntax_error_t err = {
        .line = pos.line,
        .column = pos.column,
        .pass = tokenizing_pass,
        .message =
            "Reached the end of the file before reaching the end of a comment.",
        .hint =
            "Multi-line comments should be terminated with a */ at the end."};
    dynarray_append(s->errors, &err);
  }
  if (multiline) {
    s->pos += 1;
  }
  return (token_t){
      .type = ika_comment,
      .value = tokenizer_extract_value(s, starting_offset, s->pos),
      .position = tokenizer_calculate_position(s->source, starting_offset)};
}

// Match the largest possible operator.  So '>=' gets matched before '>'
static token_t tokenize_operator(tokenizer_input_stream *s) {
  int matched_op = ika_unknown;
  uint32_t largest_matched_op_str_len = 0;
  for (uint32_t i = __ika_operators_start + 1; i < __ika_operators_end; i++) {
    str type = cstr((char *)ika_base_type_table[i].txt);
    if (type.length < s->source.length - s->pos) {
      if (str_matches_at_index(s->source, type, s->pos)) {
        if (type.length > largest_matched_op_str_len) {
          matched_op = i;
          largest_matched_op_str_len = type.length;
        }
      }
    }
  }
  token_t token = (token_t){
      .type = matched_op,
      .value = cstr((char *)ika_base_type_table[matched_op].txt),
      .position = tokenizer_calculate_position(s->source, s->pos),
  };
  s->pos += largest_matched_op_str_len;
  return token;
}

static e_ika_type lookup_ika_type_or_use_default(str value,
                                                 e_ika_type _default) {
  for (int i = __ika_types_start + 1; i < __ika_types_end; i++) {
    if (str_eq(cstr(ika_base_type_table[i].txt), value))
      return ika_base_type_table[i].type;
  }
  for (int i = __ika_keywords_start + 1; i < __ika_keywords_end; i++) {
    if (str_eq(cstr(ika_base_type_table[i].txt), value))
      return ika_base_type_table[i].type;
  }
  // Handle true and false reserved words  TODO: Mark them reserved somehow
  if (str_eq(value, cstr("true")) || str_eq(value, cstr("false")))
    return ika_bool_literal;
  return _default;
}

static token_t tokenize_identifier(tokenizer_input_stream *s) {
  if (!is_alpha(current_char(s))) {
    printf("tokenize_identifier got called with a %c\n", current_char(s));
    printf("at stream position %d\n", s->pos);
    printf("Line: %d\n", tokenizer_calculate_position(s->source, s->pos).line);
    assert(false);
  }
  int starting_offset = s->pos;
  do {
    s->pos++;
  } while (is_alpha_numeric(current_char(s)) && s->pos < s->source.length);
  str value = tokenizer_extract_value(s, starting_offset, s->pos);
  return (token_t){
      .type = lookup_ika_type_or_use_default(value, ika_identifier),
      .value = tokenizer_extract_value(s, starting_offset, s->pos),
      .position = tokenizer_calculate_position(s->source, starting_offset)};
}

static token_t tokenize_numeric(tokenizer_input_stream *s) {
  assert(is_digit_marker(s));
  int starting_offset = s->pos;
  do {
    s->pos++;
  } while (is_numeric(s) && s->pos < s->source.length);
  str value = tokenizer_extract_value(s, starting_offset, s->pos);
  return (token_t){
      .type =
          str_contains(value, cstr(".")) ? ika_float_literal : ika_int_literal,
      .position = tokenizer_calculate_position(s->source, starting_offset),
      .value = value,
  };
}

dynarray *tokenizer_scan(allocator_t allocator, str source, dynarray *errors) {
  dynarray *tokens = dynarray_init(allocator, sizeof(token_t));
  tokenizer_input_stream s = {
      .allocator = allocator, .source = source, .pos = 0, .errors = errors};

  while (s.pos < s.source.length) {
    if (is_comment(&s)) {
      token_t token = tokenize_comment(&s);
      dynarray_append(tokens, &token);
    } else if (is_operator(&s)) {
      token_t token = tokenize_operator(&s);
      dynarray_append(tokens, &token);
    } else if (is_alpha(current_char(&s))) {
      token_t token = tokenize_identifier(&s);
      dynarray_append(tokens, &token);
    } else if (is_string_marker(&s)) {
      token_t token = tokenize_string(&s);
      dynarray_append(tokens, &token);
    } else if (is_digit_marker(&s)) {
      token_t token = tokenize_numeric(&s);
      dynarray_append(tokens, &token);
    } else {
      s.pos += 1;
    }
  }

  log_info("Done tokenizing, found {u64} tokens.\n", tokens->count);
  return tokens;
}

str tokenizer_get_token_type_label(token_t *t) {
  uint32_t total_types =
      sizeof(ika_base_type_table) / sizeof(*ika_base_type_table);
  for (int i = 0; i < total_types; i++) {
    if (ika_base_type_table[i].type == t->type) {
      return cstr((char *)ika_base_type_table[i].label);
    }
  }
  return cstr("unknown");
}

str tokenizer_get_token_type_name(e_ika_type type) {
  int i = 0;
  while (ika_base_type_table[i].type != ika_eof) {
    if (ika_base_type_table[i].type == type) {
      return cstr((char *)ika_base_type_table[i].label);
    }
    i++;
  }
  return cstr("type_lookup_failed");
}

void tokenizer_print_token(FILE *out, void *ptr) {
  token_t *t = (token_t *)ptr;
  fprintf(out, "%s", tokenizer_get_token_type_label(t).ptr);
  fprintf(out, " %d,%d ", t->position.column, t->position.line);
  fprintf(out, "'%.*s'", t->value.length, t->value.ptr);
}
