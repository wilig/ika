#pragma once

typedef enum {
#define TOKEN(name, _) TOKEN_##name,

  _token_types_start,
#include "tokens/types.h"
  _token_types_end,

  _token_literals_start,
#include "tokens/literals.h"
  _token_literals_end,

  _token_operators_start,
#include "tokens/operators.h"
  _token_operators_end,

  _token_keywords_start,
#include "tokens/keywords.h"
  _token_keywords_end,

#include "tokens/misc.h"
  TOKEN(EOF, "")

#undef TOKEN
} e_token_type;

// Setup a scan map to use for tokenization
static char *token_char_map[] = {
#define TOKEN(name, string) [TOKEN_##name] = string,
#include "tokens/keywords.h"
#include "tokens/literals.h"
#include "tokens/misc.h"
#include "tokens/operators.h"
#include "tokens/types.h"
#undef TOKEN
};

#define TO_QUOTED_STRING(x) #x

// Create a nice pretty printing array.
static char *token_as_char[] = {
#define TOKEN(name, _) [TOKEN_##name] = TO_QUOTED_STRING(TOKEN_##name),
#include "tokens/keywords.h"
#include "tokens/literals.h"
#include "tokens/misc.h"
#include "tokens/operators.h"
#include "tokens/types.h"
#undef TOKEN
};
