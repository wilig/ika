// clang-format off


// Ika operator list
// Defines the enum name, the token it scans from, and a pretty print value.
//
// Warning: Order matters here, tokens should be ordered by scan length.  This
// ensures larger tokens get matched before shorter tokens.
//
// ie.  '==' gets matched as equals before '=' gets matched as assign.
//
TOKEN(ika_keyword_print, "print", "ika_keyword_print")
TOKEN(ika_keyword_let, "let", "ika_keyword_let")
TOKEN(ika_keyword_fn, "fn", "ika_keyword_fn")
TOKEN(ika_keyword_return, "return", "ika_keyword_return")
TOKEN(ika_keyword_if, "if", "ika_keyword_if")
TOKEN(ika_keyword_else, "else", "ika_keyword_else")
