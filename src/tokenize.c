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

static bool is_atomic(char c) { // Couldn't resist X)
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ? true
                                                                      : false;
}

// Test to see if a reserved word is part of a larger identifier
static bool is_part_of_identifier(tokenizer_input_stream *s, str atom) {
  return is_atomic(str_get_char(s->source, s->pos + atom.length));
}

// Returns the largest type str that matches input.  So ':=' is a better match
// then ':'.
static int get_ika_operator(tokenizer_input_stream *s) {
  int matched_op = -1;
  uint32_t largest_matched_op_str_len = 0;
  for (uint32_t i = __ika_operators_start; i < __ika_operators_end; i++) {
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
  return matched_op;
}

// Returns the largest ika type name that is not part of a larger identifier
static int get_ika_type(tokenizer_input_stream *s) {
  int matched_type = -1;
  uint32_t largest_matched_type_str_len = 0;
  for (uint32_t i = __ika_types_start + 1; i < __ika_types_end - 1; i++) {
    str type = cstr((char *)ika_base_type_table[i].txt);
    if (type.length < s->source.length - s->pos) {
      if (str_matches_at_index(s->source, type, s->pos) &&
          !is_part_of_identifier(s, type)) {
        if (type.length > largest_matched_type_str_len) {
          matched_type = i;
          largest_matched_type_str_len = type.length;
        }
      }
    }
  }
  return matched_type;
}

// Returns the largest ika keyword that is not part of a larger identifier
static int get_ika_keyword(tokenizer_input_stream *s) {
  int matched_keyword = -1;
  uint32_t largest_matched_keyword_str_len = 0;
  for (uint32_t i = __ika_keywords_start; i < __ika_keywords_end; i++) {
    str kw = cstr((char *)ika_base_type_table[i].txt);
    if (kw.length < s->source.length - s->pos) {
      if (str_matches_at_index(s->source, kw, s->pos) &&
          !is_part_of_identifier(s, kw)) {
        if (kw.length > largest_matched_keyword_str_len) {
          matched_keyword = i;
          largest_matched_keyword_str_len = kw.length;
        }
      }
    }
  }
  return matched_keyword;
}

// Returns the largest type str that matches input.  So ':=' is a better match
// then ':'.
static int lookup_ika_reserved_word_index(tokenizer_input_stream *s) {
  int match = get_ika_operator(s);
  if (match > -1)
    return match;
  match = get_ika_type(s);
  if (match > -1)
    return match;
  return get_ika_keyword(s);
}

static char current_char(tokenizer_input_stream *s) {
  return str_get_char(s->source, s->pos);
}

static bool is_ika_reserved_word(tokenizer_input_stream *s) {
  return lookup_ika_reserved_word_index(s) != -1 ? true : false;
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

static bool is_boolean(tokenizer_input_stream *s) {
  int current_position = s->pos;
  if (current_char(s) != 't' && current_char(s) != 'f')
    return false;
  else if (str_matches_at_index(s->source, cstr("true"), s->pos)) {
    return !is_part_of_identifier(s, cstr("true"));
  } else if (str_matches_at_index(s->source, cstr("false"), s->pos)) {
    return !is_part_of_identifier(s, cstr("false"));
  }
  return false;
}

static bool is_valid_string_escape_character(tokenizer_input_stream *s) {
  char c = current_char(s);
  return c == '\\' || c == 'n' || c == 't' || c == '"' ? true : false;
}

static str tokenizer_extract_value(tokenizer_input_stream *s, uint32_t start,
                                   uint32_t end) {
  return str_substr_copy(s->allocator, s->source, start, end - start);
}

static token_t tokenize_string(tokenizer_input_stream *s) {
  assert(is_string_marker(s));
  int starting_offset = ++s->pos; // Skip the quote
  bool escaped = false;
  while ((!is_string_marker(s) || escaped) && s->pos <= s->source.length) {
    // Handle invalid escape codes
    if (escaped && !is_valid_string_escape_character(s)) { // Handle error
      displayCompilerError(s->source.ptr, s->pos,
                           "Invalid string escape sequence.  Valid sequences "
                           "are \\\\, \\n, \\t, \\\".",
                           NULL);
      exit(-1);
    }
    escaped = (!escaped && current_char(s) == '\\');
    s->pos += 1;
  }
  if (is_string_marker(s)) {
    return (token_t){
        .type = ika_str_literal,
        .value = tokenizer_extract_value(s, starting_offset, s->pos++),
        .position =
            tokenizer_calculate_position(s->source, starting_offset - 1)};
  } else {
    displayCompilerError(s->source.ptr, s->pos,
                         "Reached EOF before string termination.",
                         "Try adding a \" at the end of the string.");
    exit(-1);
  }
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
    displayCompilerError(s->source.ptr, s->pos, "Comment not terminated.",
                         "Try adding */ to the end of your comment.");
    exit(-1);
  }
  if (multiline) {
    s->pos += 1;
  }
  return (token_t){
      .type = ika_comment,
      .value = tokenizer_extract_value(s, starting_offset, s->pos),
      .position = tokenizer_calculate_position(s->source, starting_offset)};
}

static token_t tokenize_type(tokenizer_input_stream *s) {
  int type_idx = lookup_ika_reserved_word_index(s);
  assert(type_idx > -1);
  int starting_offset = s->pos;
  ika_type_map_entry_t type_entry = ika_base_type_table[type_idx];
  token_t token = {
      .type = type_entry.type,
      .value = tokenizer_extract_value(
          s, starting_offset, starting_offset + strlen(type_entry.txt)),
      .position = tokenizer_calculate_position(s->source, starting_offset)};
  // Skip past the rest of the token_t
  s->pos += strlen(type_entry.txt);
  return token;
}

// TODO: rename to scan symbol
static token_t tokenize_atom(tokenizer_input_stream *s) {
  assert(is_atomic(current_char(s)));
  int starting_offset = s->pos;
  do {
    s->pos++;
  } while (is_atomic(current_char(s)) && s->pos < s->source.length);
  return (token_t){
      .type = ika_identifier,
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

static token_t tokenize_boolean(tokenizer_input_stream *s) {
  assert(is_boolean(s));
  uint32_t starting_offset = s->pos;
  while (str_get_char(s->source, s->pos - 1) != 'e' &&
         s->pos < s->source.length) {
    s->pos++;
  }
  return (token_t){
      .type = ika_bool_literal,
      .position = tokenizer_calculate_position(s->source, starting_offset),
      .value = tokenizer_extract_value(s, starting_offset, s->pos)};
}

dynarray *tokenizer_scan(tokenizer_input_stream *s) {
  dynarray *tokens = dynarray_init(s->allocator, sizeof(token_t));

  while (s->pos < s->source.length) {
    if (is_comment(s)) {
      token_t token = tokenize_comment(s);
      dynarray_append(tokens, &token);
    } else if (is_ika_reserved_word(s)) {
      token_t token = tokenize_type(s);
      dynarray_append(tokens, &token);
    } else if (is_boolean(s)) { // Must be checked before atomic/identifier
      token_t token = tokenize_boolean(s);
      dynarray_append(tokens, &token);
    } else if (is_atomic(current_char(s))) {
      token_t token = tokenize_atom(s);
      dynarray_append(tokens, &token);
    } else if (is_string_marker(s)) {
      token_t token = tokenize_string(s);
      dynarray_append(tokens, &token);
    } else if (is_digit_marker(s)) {
      token_t token = tokenize_numeric(s);
      dynarray_append(tokens, &token);
    } else {
      s->pos += 1;
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
