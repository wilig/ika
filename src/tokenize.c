#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/assert.h"
#include "../lib/format.h"

#include "defines.h"
#include "errors.h"
#include "helpers.h"
#include "tokenize.h"
#include "tokens.h"
#include "types.h"

static token_position_t tokenizer_calculate_position(char *source, u32 offset) {
  int line = 0;
  int column = 0;
  for (u32 x = 0; x < offset; x++) {
    if (source[x] == '\n') {
      line++;
      column = 0;
    } else {
      column++;
    }
  }
  return (token_position_t){.column = (u32)column, .line = (u32)line};
}

static void tokenization_error(tokenizer_input_stream_t *s, const char *fmt,
                               ...) {
  token_position_t pos = tokenizer_calculate_position(s->source, s->pos);
  syntax_error_t err = {
      .line = pos.line, .column = pos.column, .pass = TOKENIZE};
  va_list args;
  va_start(args, fmt);
  err.message = format(fmt, args);
  darray_append(s->errors, err);
  va_end(args);
}

static int is_newline(tokenizer_input_stream_t *s) {
  ASSERT_MSG((s->pos <= s->source_length), "Reading past end of source.")
  char c = s->source[s->pos];
  return (c == '\n');
}

static bool is_alpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ? true
                                                                      : false;
}

static bool is_alpha_numeric(char c) { // Couldn't resist X)
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ||
                 (c >= '0' && c <= '9')
             ? true
             : false;
}

static char current_char(tokenizer_input_stream_t *s) {
  ASSERT_MSG((s->pos + 1 <= s->source_length), "Reading past end of source.")
  return s->source[s->pos];
}

static bool is_string_marker(tokenizer_input_stream_t *s) {
  ASSERT_MSG((s->pos + 1 <= s->source_length), "Reading past end of source.")
  char c = s->source[s->pos];
  return c == '"' ? true : false;
}

static bool is_comment(tokenizer_input_stream_t *s) {
  ASSERT_MSG((s->pos + 1 <= s->source_length), "Reading past end of source.")
  return streq_n(&s->source[s->pos], "//", 2) ||
         streq_n(&s->source[s->pos], "/*", 2);
}

static bool begins_multiline_comment(tokenizer_input_stream_t *s) {
  ASSERT_MSG((s->pos + 1 <= s->source_length), "Reading past end of source.")
  return streq_n(&s->source[s->pos], "/*", 2);
}

static bool ends_multiline_comment(tokenizer_input_stream_t *s) {
  ASSERT_MSG((s->pos + 1 <= s->source_length), "Reading past end of source.")
  return streq_n(&s->source[s->pos], "*/", 2);
}

static bool is_digit_marker(tokenizer_input_stream_t *s) {
  char c = current_char(s);
  return c >= '0' && c <= '9' ? true : false;
}

static bool is_numeric(tokenizer_input_stream_t *s) {
  char c = current_char(s);
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
                 (c >= 'A' && c <= 'F') || c == 'x' || c == 'o' || c == '.' ||
                 c == '-'
             ? true
             : false;
}

static bool is_valid_string_escape_character(tokenizer_input_stream_t *s) {
  char c = current_char(s);
  // If you update this list, be sure to update the error message
  return c == '\\' || c == 'n' || c == 't' || c == '"' ? true : false;
}

static char *tokenizer_extract_value(tokenizer_input_stream_t *s,
                                     uint32_t start, uint32_t end) {
  u64 len = (end - start) + 1;
  char *buffer = imust_alloc(len);
  strncpy(buffer, &s->source[start], len - 1);
  buffer[len] = 0; // Ensure the string is null terminated
  return buffer;
}

static bool is_operator(tokenizer_input_stream_t *s) {
  char ch = current_char(s);
  for (int i = _token_operators_start + 1; i < _token_operators_end - 1; i++) {
    if (ch == token_char_map[i][0]) {
      return true;
    }
  }
  return false;
}

static token_t tokenize_string(tokenizer_input_stream_t *s) {
  ASSERT(is_string_marker(s))
  u32 starting_offset = ++s->pos; // Skip the quote
  bool escaped = false;
  while ((!is_string_marker(s) || escaped) && s->pos <= s->source_length) {
    // Handle invalid escape codes
    if (escaped && !is_valid_string_escape_character(s)) { // Handle error
      tokenization_error(
          s,
          "Invalid string escape character: '\\%c'\n\n"
          "Only certain characters are allowed to be escaped within a "
          "string.\n\n "
          "The '\\' character has special meaning within a string, it "
          "causes the next character to be treated differently.  If you "
          "were just trying to use a '\\', simply use two '\\\\'.  Other "
          "supported escape characters are '\\n', '\\t', and '\\\".",
          current_char(s));
    } else {
      escaped = (!escaped && current_char(s) == '\\');
    }
    s->pos += 1;
  }
  if (!is_string_marker(s)) {
    tokenization_error(
        s, "Unterminated string error.\n\nYou must finish a string by used a "
           "closing \" at the end.  Example:  \"Hello world\"");
  }
  return (token_t){
      .type = TOKEN_STR_LITERAL,
      .value = tokenizer_extract_value(s, starting_offset, s->pos++),
      .position = tokenizer_calculate_position(s->source, starting_offset - 1)};
}

static token_t tokenize_comment(tokenizer_input_stream_t *s) {
  ASSERT(is_comment(s))
  u32 starting_offset = s->pos;
  bool multiline = begins_multiline_comment(s);
  int comment_depth = 1;
  s->pos += 2;
  while (comment_depth > 0 && s->pos <= s->source_length) {
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
    tokenization_error(s, "Untermined comment.\n\nComments must be terminated "
                          "before the end of the file.");
  }
  if (multiline) {
    s->pos += 1;
  }
  return (token_t){
      .type = TOKEN_COMMENT,
      .value = tokenizer_extract_value(s, starting_offset, s->pos),
      .position = tokenizer_calculate_position(s->source, starting_offset)};
}

static token_t tokenize_operator(tokenizer_input_stream_t *s) {
  e_token_type matched_op = TOKEN_UNKNOWN;
  u64 token_length = 0;
  for (e_token_type type = _token_operators_start + 1;
       type < _token_operators_end; type++) {
    token_length = strnlen(token_char_map[type], 100);
    if (streq_n(&s->source[s->pos], token_char_map[type], token_length)) {
      matched_op = type;
      break;
    }
  }
  token_t token = (token_t){
      .type = matched_op,
      .value = token_char_map[matched_op],
      .position = tokenizer_calculate_position(s->source, s->pos),
  };

  s->pos += token_length;
  return token;
}

static e_token_type lookup_ika_type_or_use_default(char *value,
                                                   e_token_type _default) {
  for (e_token_type type = _token_types_start + 1; type < _token_types_end;
       type++) {
    if (streq(value, token_char_map[type]))
      return type;
  }
  for (e_token_type type = _token_keywords_start + 1;
       type < _token_keywords_end; type++) {
    if (streq(value, token_char_map[type]))
      return type;
  }
  // Handle true and false reserved words
  // TODO: Mark them reserved somehow
  if (streq(value, "true") || streq(value, "false"))
    return TOKEN_BOOL_LITERAL;
  return _default;
}

static token_t tokenize_identifier(tokenizer_input_stream_t *s) {
  ASSERT_MSG(is_alpha(current_char(s)),
             "tokenize_identifer called with a non_alpha character")
  u32 starting_offset = s->pos;
  do {
    s->pos++;
  } while (is_alpha_numeric(current_char(s)) && s->pos < s->source_length);
  char *value = tokenizer_extract_value(s, starting_offset, s->pos);
  return (token_t){
      .type = lookup_ika_type_or_use_default(value, TOKEN_SYMBOL),
      .value = tokenizer_extract_value(s, starting_offset, s->pos),
      .position = tokenizer_calculate_position(s->source, starting_offset)};
}

static token_t tokenize_numeric(tokenizer_input_stream_t *s) {
  ASSERT(is_digit_marker(s))
  u32 starting_offset = s->pos;
  do {
    s->pos++;
  } while (is_numeric(s) && s->pos < s->source_length);
  char *value = tokenizer_extract_value(s, starting_offset, s->pos);
  return (token_t){
      .type =
          strchr(value, '.') == NULL ? TOKEN_INT_LITERAL : TOKEN_FLOAT_LITERAL,
      .position = tokenizer_calculate_position(s->source, starting_offset),
      .value = value,
  };
}

da_tokens *tokenizer_scan(char *source, u64 source_length,
                          da_syntax_errors *errors) {
  da_tokens *tokens = darray_init(token_t);
  tokenizer_input_stream_t s = {.source = source,
                                .source_length = source_length,
                                .pos = 0,
                                .errors = errors};

  while (s.pos < s.source_length) {
    if (is_comment(&s)) {
      token_t token = tokenize_comment(&s);
      darray_append(tokens, token);
    } else if (is_operator(&s)) {
      token_t token = tokenize_operator(&s);
      darray_append(tokens, token);
    } else if (is_alpha(current_char(&s))) {
      token_t token = tokenize_identifier(&s);
      darray_append(tokens, token);
    } else if (is_string_marker(&s)) {
      token_t token = tokenize_string(&s);
      darray_append(tokens, token);
    } else if (is_digit_marker(&s)) {
      token_t token = tokenize_numeric(&s);
      darray_append(tokens, token);
    } else {
      s.pos += 1;
    }
  }

  return tokens;
}

const char *tokenizer_get_token_type_label(token_t *t) {
  return token_as_char[t->type];
}

const char *tokenizer_get_token_type_name(e_token_type type) {
  return token_as_char[type];
}

void tokenizer_print_token(FILE *out, token_t *t) {
  fprintf(out, "%s", token_as_char[t->type]);
  fprintf(out, " %d,%d ", t->position.column, t->position.line);
  fprintf(out, "'%s'", t->value);
}
