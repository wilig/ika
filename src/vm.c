#include <stdio.h>

#include "../lib/allocator.h"
#include "../lib/assert.h"

#include "rt/darray.h"

#include "vm.h"

static ika_value op(int op) { return (ika_value){.type = IKA_OP, .value = op}; }

static ika_value integer(int value) {
  return (ika_value){.type = IKA_INT, .value = value};
}

static ika_value phloat(float value) {
  return (ika_value){.type = IKA_FLOAT, .value = value};
}

static ika_value string(char *value) {
  return (ika_value){.type = IKA_STR, .value = (i64)value};
}

static void ika_add(ika_value *stack) {
  ika_value r1, r2;
  darray_pop(stack, &r1);
  darray_pop(stack, &r2);
  ASSERT_MSG((r1.type == r2.type), "Type mismatch in vm opcode.  Compiler bug");
  switch (r1.type) {
  case IKA_INT: {
    ika_value res = {.type = IKA_INT, .value = (i64)r1.value + (i64)r2.value};
    darray_push(stack, res);
    break;
  }
  case IKA_FLOAT: {
    ika_value res = {.type = IKA_FLOAT, .value = (f64)r1.value + (f64)r2.value};
    darray_push(stack, res);
    break;
  }
  case IKA_STR:
    ASSERT_MSG((FALSE), "Cannot add strings");
    break;
  case IKA_OP:
    ASSERT(FALSE);
    break;
  }
}

static void ika_mul(ika_value *stack) {
  ika_value r1, r2;
  darray_pop(stack, &r1);
  darray_pop(stack, &r2);
  ASSERT_MSG((r1.type == r2.type), "Type mismatch in vm opcode.  Compiler bug");
  switch (r1.type) {
  case IKA_INT: {
    ika_value res = {.type = IKA_INT, .value = (i64)r1.value * (i64)r2.value};
    darray_push(stack, res);
    break;
  }
  case IKA_FLOAT: {
    ika_value res = {.type = IKA_FLOAT, .value = (f64)r1.value * (f64)r2.value};
    darray_push(stack, res);
    break;
  }
  case IKA_STR:
    ASSERT_MSG((FALSE), "Cannot mul strings");
    break;
  case IKA_OP:
    ASSERT(FALSE);
    break;
  }
}

static void ika_print(ika_value *stack) {
  ika_value v;
  darray_pop(stack, &v);
  switch (v.type) {
  case IKA_INT:
    printf("%li\n", (i64)v.value);
    break;
  case IKA_FLOAT:
    printf("%f\n", (double)v.value);
    break;
  case IKA_STR:
    printf("%s\n", (char *)v.value);
    break;
  case IKA_OP:
    ASSERT_MSG((FALSE), "Tried to print an OP");
    break;
  }
}

void execute(ika_value *codez) {
  ika_value *stack = darray_init(ika_value);
  u64 i = 0;
  u32 exit = FALSE;
  do {
    ika_value opcode = codez[i];
    switch (opcode.type) {
    case IKA_OP:
      switch (opcode.value) {
      case ADD: {
        ika_add(stack);
        break;
      }
      case MUL: {
        ika_mul(stack);
        break;
      }
      case PRINT:
        ika_print(stack);
        break;
      case EOP:
        exit = TRUE;
        break;
      default:
        printf("opcode %li not implemented yet", opcode.value);
        break;
      }
      break;
    default:
      darray_append(stack, opcode);
    }
    i++;
  } while (exit != TRUE);
}

/* int main(int argc, char **argv) { */
/*   initialize_allocator(); */
/*   printf("vm startup\n\n"); */

/*   char *message = "Result is:"; */

/*   ika_value codez[] = {integer(10), integer(20), op(ADD), */
/*                        integer(45), op(MUL),     string(message), */
/*                        op(PRINT),   op(PRINT),   op(EOP)}; */
/*   execute(codez); */

/*   printf("\nvm shutdown\n\n"); */

/*   shutdown_allocator(); */
/* } */
