#include "tokenize.h"
#include "allocator.h"
#include "defines.h"
#include "dynarray.h"
#include "errors.h"
#include "log.h"
#include "str.h"
#include "types.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  // position->column = column;
  // position->line = line;
}

int is_whitespace(tokenizer_input_stream *s) {
  char c = str_get_char(s->source, s->pos);
  if (c == '\t' || c == ' ' || c == '\n') {
    return true;
  };
  return false;
}

int is_newline(tokenizer_input_stream *s) {
  char c = str_get_char(s->source, s->pos);
  if (c == '\n') {
    return true;
  } else {
    return false;
  }
}

// This is quite slow and will probably be a major bottleneck,
// but get it to work fist, then make it fast.
// Returns the largest type str that matches input.  So ':=' is a better match
// then ':'.
int lookup_ika_type_index(tokenizer_input_stream *s) {
  uint32_t size = sizeof(ika_base_type_table) / sizeof(*ika_base_type_table);
  uint32_t matched_type = -1;
  uint32_t largested_matched_type_str_len = 0;
  for (uint32_t i = 0; i < size; i++) {
    str type = cstr((char *)ika_base_type_table[i].txt);
    // uint32_t type_str_len = strlen(type_str);
    if (type.length < s->source.length - s->pos) {
      if (str_matches_at_index(s->source, type, s->pos)) {
        if (type.length > largested_matched_type_str_len) {
          matched_type = i;
          largested_matched_type_str_len = type.length;
        }
      }
    }
  }
  return matched_type;
}

static char current_char(tokenizer_input_stream *s) {
  return str_get_char(s->source, s->pos);
}

static bool is_ika_type(tokenizer_input_stream *s) {
  return lookup_ika_type_index(s) != -1 ? true : false;
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

static bool is_atomic(tokenizer_input_stream *s) { // Couldn't resist X)
  char c = current_char(s);
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ? true
                                                                      : false;
}

static bool is_boolean(tokenizer_input_stream *s) {
  int current_position = s->pos;
  if (str_matches_at_index(s->source, cstr("true"), s->pos) ||
      str_matches_at_index(s->source, cstr("false"), s->pos)) {
    s->pos += 4;
    if (str_matches_at_index(s->source, cstr("e"), s->pos))
      s->pos++;
    // Handle falsey, trueth named identifiers
    bool is_part_of_identifier = is_atomic(s);
    s->pos = current_position;
    return !is_part_of_identifier;
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
      // TODO:  Move this to tokenize.c and take tokenizer_input_stream as the
      // parameter.
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
  int type_idx = lookup_ika_type_index(s);
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
  assert(is_atomic(s));
  int starting_offset = s->pos;
  do {
    s->pos++;
  } while (is_atomic(s) && s->pos < s->source.length);
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
  return (token_t){
      .type = ika_num_literal,
      .position = tokenizer_calculate_position(s->source, starting_offset),
      .value = tokenizer_extract_value(s, starting_offset, s->pos)};
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
    } else if (is_ika_type(s)) {
      token_t token = tokenize_type(s);
      dynarray_append(tokens, &token);
    } else if (is_boolean(s)) { // Must be checked before atomic/identifier
      token_t token = tokenize_boolean(s);
      dynarray_append(tokens, &token);
    } else if (is_atomic(s)) {
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

  printf("Done tokenizing, found %li tokens.\n", tokens->count);
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
