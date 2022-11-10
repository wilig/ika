#pragma once

#include "types.h"

typedef enum {
#define TOKEN(name, _, __) TOKEN_##name,

  __types_start,
#include "tokens/types.h"
  __types_stop,

  __literals_start,
#include "tokens/literals.h"
  __literals_stop,

  __operators_start,
#include "tokens/operators.h"
  __operators_stop,

  __keywords_start,
#include "tokens/keywords.h"
  __keywords_end,

#include "tokens/misc.h"
  TOKEN_EOF,

#undef TOKEN
} e_token_type;

// Setup a scan map to use for tokenization
static char *token_char_map[] = {
#define TOKEN(name, string, _) [TOKEN_##name] = string,
#include "tokens/keywords.h"
#include "tokens/literals.h"
#include "tokens/misc.h"
#include "tokens/operators.h"
#include "tokens/types.h"
#undef TOKEN
};

// Create a nice pretty printing array.
static char *token_as_char[] = {
#define TOKEN(name, string, _) [TOKEN_##name] = "TOKEN_##name",
#include "tokens/keywords.h"
#include "tokens/literals.h"
#include "tokens/misc.h"
#include "tokens/operators.h"
#include "tokens/types.h"
#undef TOKEN
};
