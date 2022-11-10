#pragma once

/* typedef enum { */
/*   TOKEN_UNKNOWN = 0, */

/*   __ika_types_start, */
/*   TOKEN_INT, */
/*   TOKEN_FLOAT, */
/*   TOKEN_BOOL, */
/*   TOKEN_STR, */
/*   TOKEN_RUNE, */
/*   TOKEN_ANY, */
/*   TOKEN_VOID, */
/*   __ika_types_end, */

/*   TOKEN_SYMBOL, */

/*   __ika_literal_start, */
/*   TOKEN_INT_LITERAL, */
/*   TOKEN_FLOAT_LITERAL, */
/*   TOKEN_STR_LITERAL, */
/*   TOKEN_BOOL_LITERAL, */
/*   __ika_literal_end, */

/*   __ika_operators_start, */
/*   // Optimization:  Order these by token string length, this allows the */
/*   // tokenizer to search the list linearly and take the first match. */
/*   TOKEN_GTE, */
/*   TOKEN_LTE, */
/*   TOKEN_EQL, */
/*   TOKEN_NEQ, */
/*   TOKEN_ADD, */
/*   TOKEN_SUB, */
/*   TOKEN_MUL, */
/*   TOKEN_QUO, */
/*   TOKEN_MOD, */
/*   TOKEN_BANG, */
/*   TOKEN_ASSIGN, */
/*   TOKEN_PAREN_OPEN, */
/*   TOKEN_PAREN_CLOSE, */
/*   TOKEN_BRACE_OPEN, */
/*   TOKEN_BRACE_CLOSE, */
/*   TOKEN_BOX_OPEN, */
/*   TOKEN_BOX_CLOSE, */
/*   TOKEN_SEMI_COLON, */
/*   TOKEN_COLON, */
/*   TOKEN_COMMA, */
/*   TOKEN_DOT, */
/*   TOKEN_GT, */
/*   TOKEN_LT, */
/*   __ika_operators_end, */

/*   __ika_keywords_start, */
/*   TOKEN_KEYWORD_PRINT, */
/*   TOKEN_KEYWORD_LET, */
/*   TOKEN_KEYWORD_FN, */
/*   TOKEN_KEYWORD_RETURN, */
/*   TOKEN_KEYWORD_IF, */
/*   TOKEN_KEYWORD_ELSE, */
/*   TOKEN_KEYWORD_BEHAVIOR, */
/*   TOKEN_KEYWORD_REQUIRES, */
/*   __ika_keywords_end, */

/*   TOKEN_COMMENT, */
/*   TOKEN_EOF, */
/* } e_token_type; */

/* typedef struct { */
/*   const char *txt; */
/*   e_token_type type; */
/*   const char *label; */
/* } ika_type_map_entry_t; */

/* static char *ika_type_to_char_map[] = {[TOKEN_UNKNOWN] = "unknown", */
/*                                        [__ika_types_start] = "<!illegal>", */
/*                                        [TOKEN_INT] = "int", */
/*                                        [TOKEN_FLOAT] = "float", */
/*                                        [TOKEN_BOOL] = "bool", */
/*                                        [TOKEN_STR] = "str", */
/*                                        [TOKEN_RUNE] = "rune", */
/*                                        [TOKEN_ANY] = "any", */
/*                                        [TOKEN_VOID] = "void", */
/*                                        [__ika_types_end] = "<!illegal>", */
/*                                        [TOKEN_SYMBOL] = "symbol", */
/*                                        [__ika_literal_start] = "<!illegal>",
 */
/*                                        [TOKEN_INT_LITERAL] = "int literal",
 */
/*                                        [TOKEN_FLOAT_LITERAL] = "float
 * literal", */
/*                                        [TOKEN_STR_LITERAL] = "str literal",
 */
/*                                        [TOKEN_BOOL_LITERAL] = "bool literal",
 */
/*                                        [__ika_literal_end] = "<!illegal>", */
/*                                        [__ika_operators_start] =
 * "<!illegal>", */
/*                                        [TOKEN_GTE] = ">=", */
/*                                        [TOKEN_LTE] = "<=", */
/*                                        [TOKEN_EQL] = "==", */
/*                                        [TOKEN_NEQ] = "!=", */
/*                                        [TOKEN_ADD] = "+", */
/*                                        [TOKEN_SUB] = "-", */
/*                                        [TOKEN_MUL] = "*", */
/*                                        [TOKEN_QUO] = "/", */
/*                                        [TOKEN_MOD] = "%", */
/*                                        [TOKEN_BANG] = "!", */
/*                                        [TOKEN_ASSIGN] = "=", */
/*                                        [TOKEN_PAREN_OPEN] = "(", */
/*                                        [TOKEN_PAREN_CLOSE] = ")", */
/*                                        [TOKEN_BRACE_OPEN] = "{", */
/*                                        [TOKEN_BRACE_CLOSE] = "}", */
/*                                        [TOKEN_BOX_OPEN] = "[", */
/*                                        [TOKEN_BOX_CLOSE] = "]", */
/*                                        [TOKEN_SEMI_COLON] = ";", */
/*                                        [TOKEN_COLON] = ":", */
/*                                        [TOKEN_COMMA] = ",", */
/*                                        [TOKEN_DOT] = ".", */
/*                                        [TOKEN_GT] = ">", */
/*                                        [TOKEN_LT] = "<", */
/*                                        [__ika_operators_end] = "<!illegal>",
 */
/*                                        [__ika_keywords_start] = "<!illegal>",
 */
/*                                        [TOKEN_KEYWORD_PRINT] = "print", */
/*                                        [TOKEN_KEYWORD_LET] = "let", */
/*                                        [TOKEN_KEYWORD_FN] = "fn", */
/*                                        [TOKEN_KEYWORD_RETURN] = "return", */
/*                                        [TOKEN_KEYWORD_IF] = "if", */
/*                                        [TOKEN_KEYWORD_ELSE] = "else", */
/*                                        [TOKEN_KEYWORD_BEHAVIOR] = "behavior",
 */
/*                                        [TOKEN_KEYWORD_REQUIRES] = "requires",
 */
/*                                        [__ika_keywords_end] = "<!illegal>",
 */
/*                                        [TOKEN_COMMENT] = "comment", */
/*                                        [TOKEN_EOF] = "eof"}; */

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
