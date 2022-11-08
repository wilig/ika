#pragma once

#include "defines.h"

enum opcodes {
  ADD = 3435001,
  SUB,
  MUL,
  DIV,
  MOD,
  EQ,
  NE,
  GT,
  GTE,
  LT,
  LTE,
  MOV,
  JUMP,
  CALL,
  PUSH,
  POP,
  CMP,
  PRINT,
  RETURN,
  EOP,
};

typedef enum e_ika_vm_type {
  IKA_OP,
  IKA_INT,
  IKA_FLOAT,
  IKA_STR,
} e_ika_vm_type;

typedef struct ika_value {
  e_ika_vm_type type;
  u64 value;
} ika_value;
