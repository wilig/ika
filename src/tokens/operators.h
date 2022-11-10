// clang-format off


// Ika operator list
// Defines the name, the characters it scans from (if any).
//
// Warning: Order matters here, tokens should be ordered by scan length.  This
// ensures larger tokens get matched before shorter tokens.
//
// ie.  '==' gets matched as equals before '=' gets matched as assign.
//
TOKEN(GTE,         ">=")
TOKEN(LTE,         "<=")
TOKEN(EQL,         "==")
TOKEN(NEQ,         "!=")
TOKEN(GT,          ">")
TOKEN(LT,          "<")
TOKEN(ADD,         "+")
TOKEN(SUB,         "-")
TOKEN(MUL,         "*")
TOKEN(QUO,         "/")
TOKEN(MOD,         "%")
TOKEN(BANG,        "!")
TOKEN(COLON,       ":")
TOKEN(ASSIGN,      "=")
TOKEN(PAREN_OPEN,  "(")
TOKEN(PAREN_CLOSE, ")")
TOKEN(BRACE_OPEN,  "{")
TOKEN(BRACE_CLOSE, "}")
TOKEN(BOX_OPEN,    "[")
TOKEN(BOX_CLOSE,   "]")
TOKEN(SEMI_COLON,  ";")
TOKEN(COMMA,       ",")
TOKEN(DOT,         ".")
