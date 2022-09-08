#include "tokenize.h"
#include "allocator.h"
#include "defines.h"
#include "errors.h"
#include "log.h"
#include "str.h"
#include "types.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void tokenizer_calculate_position(str source, uint32_t offset,
                                  token_position_t *position) {
  int line = 0;
  int column = 0;
  for (int x = 0; x < offset; x++) {
    if (str_get_char(source, x) == '\n') {
      line++;
      column = 0;
    }
    column++;
  }
  position->column = column;
  position->line = line;
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

char current_char(tokenizer_input_stream *s) {
  return str_get_char(s->source, s->pos);
}

bool is_ika_type(tokenizer_input_stream *s) {
  return lookup_ika_type_index(s) != -1 ? true : false;
}

bool is_string_marker(tokenizer_input_stream *s) {
  char c = str_get_char(s->source, s->pos);
  return c == '"' ? true : false;
}

bool is_comment(tokenizer_input_stream *s) {
  return str_matches_at_index(s->source, cstr("//"), s->pos) ||
         str_matches_at_index(s->source, cstr("/*"), s->pos);
}

bool begins_multiline_comment(tokenizer_input_stream *s) {
  return str_matches_at_index(s->source, cstr("/*"), s->pos);
}

bool ends_multiline_comment(tokenizer_input_stream *s) {
  return str_matches_at_index(s->source, cstr("*/"), s->pos);
}

bool is_digit_marker(tokenizer_input_stream *s) {
  char c = current_char(s);
  return c >= '0' && c <= '9' ? true : false;
}

bool is_numeric(tokenizer_input_stream *s) {
  char c = current_char(s);
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
                 (c >= 'A' && c <= 'F') || c == 'x' || c == 'o' || c == '.' ||
                 c == '-'
             ? true
             : false;
}

bool is_atomic(tokenizer_input_stream *s) { // Couldn't resist X)
  char c = current_char(s);
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ? true
                                                                      : false;
}

bool is_boolean(tokenizer_input_stream *s) {
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

bool is_valid_string_escape_character(tokenizer_input_stream *s) {
  char c = current_char(s);
  return c == '\\' || c == 'n' || c == 't' || c == '"' ? true : false;
}

token_t *tokenize_new_token(allocator_t allocator) {
  allocated_memory mem = allocator_alloc(allocator, sizeof(token_t));
  if (!mem.valid) {
    log_error("Failed to allocate memory for token_t.");
    exit(-1);
  }
  token_t *token = mem.ptr;
  return token;
}

void tokenize_string(tokenizer_input_stream *s, token_t *token) {
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
    tokenizer_calculate_position(s->source, starting_offset - 1,
                                 &token->position);
    token->type = ika_str_literal;
    token->value = str_substr_copy(s->allocator, s->source, starting_offset,
                                   s->pos++ - starting_offset);
  } else {
    displayCompilerError(s->source.ptr, s->pos,
                         "Reached EOF before string termination.",
                         "Try adding a \" at the end of the string.");
    exit(-1);
  }
}

void tokenize_comment(tokenizer_input_stream *s, token_t *token) {
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
  tokenizer_calculate_position(s->source, starting_offset, &token->position);
  token->type = ika_comment;
  token->value = str_substr_copy(s->allocator, s->source, starting_offset,
                                 s->pos - starting_offset);
}

void tokenize_type(tokenizer_input_stream *s, token_t *token) {
  int type_idx = lookup_ika_type_index(s);
  assert(type_idx > -1);
  int starting_offset = s->pos;
  ika_type_map_entry_t type_entry = ika_base_type_table[type_idx];
  tokenizer_calculate_position(s->source, starting_offset, &token->position);
  token->type = type_entry.type;
  token->value =
      str_substr_copy(s->allocator, s->source, s->pos, strlen(type_entry.txt));
  // Skip past the rest of the token_t
  s->pos += strlen(type_entry.txt);
}

// TODO: rename to scan symbol
void tokenize_atom(tokenizer_input_stream *s, token_t *token) {
  assert(is_atomic(s));
  int starting_offset = s->pos;
  do {
    s->pos++;
  } while (is_atomic(s) && s->pos < s->source.length);
  tokenizer_calculate_position(s->source, starting_offset, &token->position);
  token->type = ika_identifier;
  token->value = str_substr_copy(s->allocator, s->source, starting_offset,
                                 s->pos - starting_offset);
}

void tokenize_numeric(tokenizer_input_stream *s, token_t *token) {
  assert(is_digit_marker(s));
  int starting_offset = s->pos;
  do {
    s->pos++;
  } while (is_numeric(s) && s->pos < s->source.length);
  tokenizer_calculate_position(s->source, starting_offset, &token->position);
  token->type = ika_num_literal;
  token->value = str_substr_copy(s->allocator, s->source, starting_offset,
                                 s->pos - starting_offset);
}

void tokenize_boolean(tokenizer_input_stream *s, token_t *token) {
  assert(is_boolean(s));
  int starting_offset = s->pos;
  while (str_get_char(s->source, s->pos - 1) != 'e' &&
         s->pos < s->source.length) {
    s->pos++;
  }
  tokenizer_calculate_position(s->source, starting_offset, &token->position);
  token->type = ika_bool_literal;
  token->value = str_substr_copy(s->allocator, s->source, starting_offset,
                                 s->pos - starting_offset);
}

token_t **tokenizer_scan(tokenizer_input_stream *s) {
  allocated_memory mem =
      allocator_alloc(s->allocator, sizeof(token_t) * TOKEN_BUCKET_SIZE);
  if (!mem.valid) {
    log_error("Failed to allocate memory from token_t list");
    exit(-1);
  }
  token_t **tokens = mem.ptr;
  int token_count = 0;

  while (s->pos < s->source.length) {
    if (is_comment(s)) {
      token_t *token = tokenize_new_token(s->allocator);
      tokenize_comment(s, token);
      tokens[token_count++] = token;
    } else if (is_ika_type(s)) {
      token_t *token = tokenize_new_token(s->allocator);
      tokenize_type(s, token);
      tokens[token_count++] = token;
    } else if (is_boolean(s)) { // Must be checked before atomic/identifier
      token_t *token = tokenize_new_token(s->allocator);
      tokenize_boolean(s, token);
      tokens[token_count++] = token;
    } else if (is_atomic(s)) {
      token_t *token = tokenize_new_token(s->allocator);
      tokenize_atom(s, token);
      tokens[token_count++] = token;
    } else if (is_string_marker(s)) {
      token_t *token = tokenize_new_token(s->allocator);
      tokenize_string(s, token);
      tokens[token_count++] = token;
    } else if (is_digit_marker(s)) {
      token_t *token = tokenize_new_token(s->allocator);
      tokenize_numeric(s, token);
      tokens[token_count++] = token;
    } else {
      s->pos += 1;
    }
    // Allocate blocks of tokens instead of individual tokens.
    if (mem.size <= sizeof(token_t *) * token_count) {
      mem = allocator_realloc(s->allocator, mem,
                              sizeof(token_t *) * TOKEN_BUCKET_SIZE);
      if (!mem.valid) {
        log_error("Failed to get additional memory for token_t list");
        exit(-1);
      } else {
        tokens = mem.ptr;
      }
    }
  }

  // If we exhausted all the token_t space, add one for the eof marker
  if (token_count % TOKEN_BUCKET_SIZE == 0) {
    tokens = realloc(tokens, sizeof(token_t *));
  }
  token_t *eof_token = tokenize_new_token(s->allocator);
  tokenizer_calculate_position(s->source, s->source.length,
                               &eof_token->position);
  eof_token->type = ika_eof;
  tokens[token_count++] = eof_token;

  printf("Done tokenizing, found %d tokens.\n", token_count);
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
  fprintf(out, " %d,%d ", t->position.line, t->position.column);
  fprintf(out, "'%.*s'", t->value.length, t->value.ptr);
}
