// clang-format off


// Ika operator list
// Defines the enum name, the token it scans from, and a pretty print value.
//
// Warning: Order matters here, tokens should be ordered by scan length.  This
// ensures larger tokens get matched before shorter tokens.
//
// ie.  '==' gets matched as equals before '=' gets matched as assign.
//
TOKEN(ika_gte,    ">=", "ika_gte")
TOKEN(ika_lte,    "<=", "ika_lte")
TOKEN(ika_eql,    "==", "ika_eql")
TOKEN(ika_neq,    "!=", "ika_neq")
TOKEN(ika_add,    "+",  "ika_add")
TOKEN(ika_sub,    "-",  "ika_sub")
TOKEN(ika_mul,    "*",  "ika_mul")
TOKEN(ika_quo,    "/",  "ika_quo")
TOKEN(ika_mod,    "%",  "ika_mod")
TOKEN(ika_bang,   "!",  "ika_bang")
TOKEN(ika_colon,  ":",  "ika_colon")
TOKEN(ika_assign, "=",  "ika_assign")
