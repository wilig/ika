#pragma once

typedef enum {
  ika_unknown,

  __ika_types_start,
  ika_int,
  ika_float,
  ika_bool,
  ika_str,
  ika_rune,
  ika_any,
  ika_void,
  __ika_types_end,

  ika_identifier,

  __ika_literal_start,
  ika_int_literal,
  ika_float_literal,
  ika_str_literal,
  ika_bool_literal,
  __ika_literal_end,

  __ika_operators_start,
  ika_add,
  ika_sub,
  ika_mul,
  ika_quo,
  ika_mod,
  ika_incr,
  ika_decr,
  ika_untyped_assign,
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
  ika_eql,
  ika_neq,
  ika_gt,
  ika_lt,
  ika_gte,
  ika_lte,
  __ika_operators_end,

  __ika_keywords_start,
  ika_keyword_ns,
  ika_keyword_import,
  ika_keyword_let,
  ika_keyword_fn,
  ika_keyword_var,
  ika_keyword_match,
  ika_keyword_test,
  ika_keyword_return,
  ika_keyword_if,
  ika_keyword_else,
  __ika_keywords_end,

  ika_comment,
  ika_eof,
} e_ika_type;

typedef struct {
  const char *txt;
  e_ika_type type;
  const char *label;
} ika_type_map_entry_t;

static ika_type_map_entry_t ika_base_type_table[] = {
    {.txt = "unknown", .type = ika_unknown, .label = "ika_unknown"},
    {.txt = "#", .type = __ika_types_start, .label = "__ika_types_start"},
    {.txt = "int", .type = ika_int, .label = "int"},
    {.txt = "float", .type = ika_float, .label = "float"},
    {.txt = "bool", .type = ika_bool, .label = "bool"},
    {.txt = "str", .type = ika_str, .label = "str"},
    {.txt = "rune", .type = ika_rune, .label = "rune"},
    {.txt = "any", .type = ika_any, .label = "ika_any"},
    {.txt = "void", .type = ika_void, .label = "ika_void"},
    {.txt = "<!illegal>", .type = __ika_types_end, .label = "__ika_types_end"},
    {.txt = "identifier", .type = ika_identifier, .label = "ika_identifier"},
    {.txt = "<!illegal>",
     .type = __ika_literal_start,
     .label = "__ika_literal_start"},
    {.txt = "int literal", .type = ika_int_literal, .label = "ika_int_literal"},
    {.txt = "float literal",
     .type = ika_float_literal,
     .label = "ika_float_literal"},
    {.txt = "str literal", .type = ika_str_literal, .label = "ika_str_literal"},
    {.txt = "bool literal",
     .type = ika_bool_literal,
     .label = "ika_bool_literal"},
    {.txt = "<!illegal>",
     .type = __ika_literal_end,
     .label = "__ika_literal_end"},
    {.txt = "<!illegal>",
     .type = __ika_operators_start,
     .label = "__ika_operators_start"},

    {.txt = "+", .type = ika_add, .label = "ika_add"},
    {.txt = "-", .type = ika_sub, .label = "ika_sub"},
    {.txt = "*", .type = ika_mul, .label = "ika_mul"},
    {.txt = "/", .type = ika_quo, .label = "ika_quo"},
    {.txt = "%", .type = ika_mod, .label = "ika_mod"},
    {.txt = "+=", .type = ika_incr, .label = "ika_incr"},
    {.txt = "-=", .type = ika_decr, .label = "ika_decr"},
    {.txt = ":=", .type = ika_untyped_assign, .label = "ika_untyped_assign"},
    {.txt = "!", .type = ika_bang, .label = "ika_bang"},
    {.txt = "=", .type = ika_assign, .label = "ika_assign"},
    {.txt = "(", .type = ika_paren_open, .label = "ika_paren_open"},
    {.txt = ")", .type = ika_paren_close, .label = "ika_paren_close"},
    {.txt = "{", .type = ika_brace_open, .label = "ika_brace_open"},
    {.txt = "}", .type = ika_brace_close, .label = "ika_brace_close"},
    {.txt = "[", .type = ika_box_open, .label = "ika_box_open"},
    {.txt = "]", .type = ika_box_close, .label = "ika_box_close"},
    {.txt = ";", .type = ika_semi_colon, .label = "ika_semi_colon"},
    {.txt = ":", .type = ika_colon, .label = "ika_colon"},
    {.txt = ",", .type = ika_comma, .label = "ika_comma"},
    {.txt = ".", .type = ika_dot, .label = "ika_dot"},
    {.txt = "==", .type = ika_eql, .label = "ika_eql"},
    {.txt = "!=", .type = ika_neq, .label = "ika_neq"},
    {.txt = ">", .type = ika_gt, .label = "ika_gt"},
    {.txt = "<", .type = ika_lt, .label = "ika_lt"},
    {.txt = ">=", .type = ika_gte, .label = "ika_gte"},
    {.txt = "<=", .type = ika_lte, .label = "ika_lte"},
    {.txt = "<!illegal>",
     .type = __ika_operators_end,
     .label = "__ika_operators_end"},
    {.txt = "<!illegal>",
     .type = __ika_keywords_start,
     .label = "__ika_keywords_start"},
    {.txt = "ns", .type = ika_keyword_ns, .label = "ika_keyword_ns"},
    {.txt = "import",
     .type = ika_keyword_import,
     .label = "ika_keyword_import"},
    {.txt = "let", .type = ika_keyword_let, .label = "ika_keyword_let"},
    {.txt = "fn", .type = ika_keyword_fn, .label = "ika_keyword_fn"},
    {.txt = "var", .type = ika_keyword_var, .label = "ika_keyword_var"},
    {.txt = "match", .type = ika_keyword_match, .label = "ika_keyword_match"},
    {.txt = "test", .type = ika_keyword_test, .label = "ika_keyword_test"},
    {.txt = "return",
     .type = ika_keyword_return,
     .label = "ika_keyword_return"},
    {.txt = "if", .type = ika_keyword_if, .label = "ika_keyword_if"},
    {.txt = "else", .type = ika_keyword_else, .label = "ika_keyword_else"},
    {.txt = "<!illegal>",
     .type = __ika_keywords_end,
     .label = "__ika_keywords_end"},
    {.txt = "comment", .type = ika_comment, .label = "comment"},
    {.txt = "eof", .type = ika_eof, .label = "eof"},
};
