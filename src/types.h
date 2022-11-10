#pragma once

typedef enum {
  ika_unknown = 0,

  __ika_types_start,
  ika_int,
  ika_float,
  ika_bool,
  ika_str,
  ika_rune,
  ika_any,
  ika_void,
  __ika_types_end,

  ika_symbol,

  __ika_literal_start,
  ika_int_literal,
  ika_float_literal,
  ika_str_literal,
  ika_bool_literal,
  __ika_literal_end,

  __ika_operators_start,
  // Optimization:  Order these by token string length, this allows the
  // tokenizer to search the list linearly and take the first match.
  ika_gte,
  ika_lte,
  ika_eql,
  ika_neq,
  ika_add,
  ika_sub,
  ika_mul,
  ika_quo,
  ika_mod,
  ika_bang,
  ika_assign,
  ika_paren_open,
  ika_paren_close,
  ika_brace_open,
  ika_brace_close,
  ika_box_open,
  ika_box_close,
  ika_semi_colon,
  ika_colon,
  ika_comma,
  ika_dot,
  ika_gt,
  ika_lt,
  __ika_operators_end,

  __ika_keywords_start,
  ika_keyword_print,
  ika_keyword_let,
  ika_keyword_fn,
  ika_keyword_return,
  ika_keyword_if,
  ika_keyword_else,
  ika_keyword_behavior,
  ika_keyword_requires,
  __ika_keywords_end,

  ika_comment,
  ika_eof,
} e_ika_type;

typedef struct {
  const char *txt;
  e_ika_type type;
  const char *label;
} ika_type_map_entry_t;

static char *ika_type_to_char_map[] = {[ika_unknown] = "unknown",
                                       [__ika_types_start] = "<!illegal>",
                                       [ika_int] = "int",
                                       [ika_float] = "float",
                                       [ika_bool] = "bool",
                                       [ika_str] = "str",
                                       [ika_rune] = "rune",
                                       [ika_any] = "any",
                                       [ika_void] = "void",
                                       [__ika_types_end] = "<!illegal>",
                                       [ika_symbol] = "symbol",
                                       [__ika_literal_start] = "<!illegal>",
                                       [ika_int_literal] = "int literal",
                                       [ika_float_literal] = "float literal",
                                       [ika_str_literal] = "str literal",
                                       [ika_bool_literal] = "bool literal",
                                       [__ika_literal_end] = "<!illegal>",
                                       [__ika_operators_start] = "<!illegal>",
                                       [ika_gte] = ">=",
                                       [ika_lte] = "<=",
                                       [ika_eql] = "==",
                                       [ika_neq] = "!=",
                                       [ika_add] = "+",
                                       [ika_sub] = "-",
                                       [ika_mul] = "*",
                                       [ika_quo] = "/",
                                       [ika_mod] = "%",
                                       [ika_bang] = "!",
                                       [ika_assign] = "=",
                                       [ika_paren_open] = "(",
                                       [ika_paren_close] = ")",
                                       [ika_brace_open] = "{",
                                       [ika_brace_close] = "}",
                                       [ika_box_open] = "[",
                                       [ika_box_close] = "]",
                                       [ika_semi_colon] = ";",
                                       [ika_colon] = ":",
                                       [ika_comma] = ",",
                                       [ika_dot] = ".",
                                       [ika_gt] = ">",
                                       [ika_lt] = "<",
                                       [__ika_operators_end] = "<!illegal>",
                                       [__ika_keywords_start] = "<!illegal>",
                                       [ika_keyword_print] = "print",
                                       [ika_keyword_let] = "let",
                                       [ika_keyword_fn] = "fn",
                                       [ika_keyword_return] = "return",
                                       [ika_keyword_if] = "if",
                                       [ika_keyword_else] = "else",
                                       [ika_keyword_behavior] = "behavior",
                                       [ika_keyword_requires] = "requires",
                                       [__ika_keywords_end] = "<!illegal>",
                                       [ika_comment] = "comment",
                                       [ika_eof] = "eof"};

/* static ika_type_map_entry_t ika_base_type_table[] = { */
/*     {.txt = "unknown", .type = ika_unknown, .label = "ika_unknown"}, */
/*     {.txt = "#", .type = __ika_types_start, .label = "__ika_types_start"}, */
/*     {.txt = "int", .type = ika_int, .label = "int"}, */
/*     {.txt = "float", .type = ika_float, .label = "float"}, */
/*     {.txt = "bool", .type = ika_bool, .label = "bool"}, */
/*     {.txt = "str", .type = ika_str, .label = "str"}, */
/*     {.txt = "rune", .type = ika_rune, .label = "rune"}, */
/*     {.txt = "any", .type = ika_any, .label = "any"}, */
/*     {.txt = "void", .type = ika_void, .label = "void"}, */
/*     {.txt = "<!illegal>", .type = __ika_types_end, .label =
 * "__ika_types_end"}, */
/*     {.txt = "symbol", .type = ika_symbol, .label = "ika_symbol"}, */
/*     {.txt = "<!illegal>", */
/*      .type = __ika_literal_start, */
/*      .label = "__ika_literal_start"}, */
/*     {.txt = "int literal", .type = ika_int_literal, .label =
 * "ika_int_literal"}, */
/*     {.txt = "float literal", */
/*      .type = ika_float_literal, */
/*      .label = "ika_float_literal"}, */
/*     {.txt = "str literal", .type = ika_str_literal, .label =
 * "ika_str_literal"}, */
/*     {.txt = "bool literal", */
/*      .type = ika_bool_literal, */
/*      .label = "ika_bool_literal"}, */
/*     {.txt = "<!illegal>", */
/*      .type = __ika_literal_end, */
/*      .label = "__ika_literal_end"}, */
/*     {.txt = "<!illegal>", */
/*      .type = __ika_operators_start, */
/*      .label = "__ika_operators_start"}, */

/*     {.txt = ">=", .type = ika_gte, .label = "ika_gte"}, */
/*     {.txt = "<=", .type = ika_lte, .label = "ika_lte"}, */
/*     {.txt = "==", .type = ika_eql, .label = "ika_eql"}, */
/*     {.txt = "!=", .type = ika_neq, .label = "ika_neq"}, */
/*     {.txt = "+", .type = ika_add, .label = "ika_add"}, */
/*     {.txt = "-", .type = ika_sub, .label = "ika_sub"}, */
/*     {.txt = "*", .type = ika_mul, .label = "ika_mul"}, */
/*     {.txt = "/", .type = ika_quo, .label = "ika_quo"}, */
/*     {.txt = "%", .type = ika_mod, .label = "ika_mod"}, */
/*     {.txt = "!", .type = ika_bang, .label = "ika_bang"}, */
/*     {.txt = "=", .type = ika_assign, .label = "ika_assign"}, */
/*     {.txt = "(", .type = ika_paren_open, .label = "ika_paren_open"}, */
/*     {.txt = ")", .type = ika_paren_close, .label = "ika_paren_close"}, */
/*     {.txt = "{", .type = ika_brace_open, .label = "ika_brace_open"}, */
/*     {.txt = "}", .type = ika_brace_close, .label = "ika_brace_close"}, */
/*     {.txt = "[", .type = ika_box_open, .label = "ika_box_open"}, */
/*     {.txt = "]", .type = ika_box_close, .label = "ika_box_close"}, */
/*     {.txt = ";", .type = ika_semi_colon, .label = "ika_semi_colon"}, */
/*     {.txt = ":", .type = ika_colon, .label = "ika_colon"}, */
/*     {.txt = ",", .type = ika_comma, .label = "ika_comma"}, */
/*     {.txt = ".", .type = ika_dot, .label = "ika_dot"}, */
/*     {.txt = ">", .type = ika_gt, .label = "ika_gt"}, */
/*     {.txt = "<", .type = ika_lt, .label = "ika_lt"}, */
/*     {.txt = "<!illegal>", */
/*      .type = __ika_operators_end, */
/*      .label = "__ika_operators_end"}, */
/*     {.txt = "<!illegal>", */
/*      .type = __ika_keywords_start, */
/*      .label = "__ika_keywords_start"}, */
/*     {.txt = "print", .type = ika_keyword_print, .label =
 * "ika_keyword_print"}, */
/*     {.txt = "let", .type = ika_keyword_let, .label = "ika_keyword_let"}, */
/*     {.txt = "fn", .type = ika_keyword_fn, .label = "ika_keyword_fn"}, */
/*     {.txt = "return", */
/*      .type = ika_keyword_return, */
/*      .label = "ika_keyword_return"}, */
/*     {.txt = "if", .type = ika_keyword_if, .label = "ika_keyword_if"}, */
/*     {.txt = "else", .type = ika_keyword_else, .label = "ika_keyword_else"},
 */
/*     {.txt = "behavior", */
/*      .type = ika_keyword_behavior, */
/*      .label = "ika_keyword_behavior"}, */
/*     {.txt = "requires", */
/*      .type = ika_keyword_requires, */
/*      .label = "ika_keyword_requires"}, */
/*     {.txt = "<!illegal>", */
/*      .type = __ika_keywords_end, */
/*      .label = "__ika_keywords_end"}, */
/*     {.txt = "comment", .type = ika_comment, .label = "comment"}, */
/*     {.txt = "eof", .type = ika_eof, .label = "eof"}, */
/* }; */
