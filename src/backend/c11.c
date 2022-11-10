#include "c11.h"
#include "../../lib/assert.h"
#include "../rt/darray.h"
#include "../rt/str_builder.h"

#include "../compiler.h"
#include <stdio.h>
#include <string.h>

static void build_node(c11_be_t *, ast_node_t *);
static void build_fn_call(c11_be_t *, ast_node_t *);

static const char *ika_type_to_c(e_token_type type) {
  return c11_op_codes[type];
}

static void build_expr(c11_be_t *b, ast_node_t *node) {
  switch (node->type) {
  case ast_expr:
    str_builder_append(b->sb, "(");
    build_expr(b, node->expr.left);
    str_builder_append(b->sb, " ");
    str_builder_append(b->sb, ika_type_to_c(node->expr.op));
    str_builder_append(b->sb, " ");
    build_expr(b, node->expr.right);
    str_builder_append(b->sb, ")");
    break;
  case ast_term:
    str_builder_append(b->sb, "(");
    build_expr(b, node->term.left);
    str_builder_append(b->sb, " ");
    str_builder_append(b->sb, ika_type_to_c(node->term.op));
    str_builder_append(b->sb, " ");
    build_expr(b, node->term.right);
    str_builder_append(b->sb, ")");
    break;
  case ast_int_literal:
    str_builder_append(b->sb, node->literal.integer_value);
    break;
  case ast_float_literal:
    str_builder_append(b->sb, node->literal.float_value);
    break;
  case ast_str_literal:
    str_builder_append(b->sb, "cstr(\"");
    str_builder_append(b->sb, node->literal.string_value);
    str_builder_append(b->sb, "\")");
    break;
  case ast_bool_literal:
    str_builder_append(b->sb, node->literal.integer_value == 0 ? cstr("FALSE")
                                                               : cstr("TRUE"));
    break;
  case ast_symbol:
    str_builder_append(b->sb, node->symbol.value);
    break;
  case ast_fn_call:
    build_fn_call(b, node);
    break;
  default:
    printf("Unknown expr type: %d\n", node->type);
    ASSERT_MSG((FALSE), "Unexpected ast_node type in build expr");
  }
}

static void build_fn_call(c11_be_t *b, ast_node_t *node) {
  ASSERT_MSG((node->type == ast_fn_call), "Expected a function call node");
  fn_call_t fn_call = node->fn_call;
  printf("c11 building '%s' function call\n", fn_call.symbol->symbol.value);
  str_builder_append(b->sb, "I_");
  str_builder_append(b->sb, fn_call.symbol->symbol.value);
  str_builder_append(b->sb, "(");
  for (int i = 0; i < darray_len(fn_call.exprs); i++) {
    build_expr(b, &fn_call.exprs[i]);
    if (i < darray_len(fn_call.exprs) - 1)
      str_builder_append(b->sb, ", ");
  }
  str_builder_append(b->sb, ")");
}

static void build_decl(c11_be_t *b, ast_node_t *node) {
  ASSERT_MSG((node->type == ast_decl), "Expected a decl node");
  decl_t decl = node->decl;
  if (decl.constant)
    str_builder_append(b->sb, "const ");
  str_builder_append(b->sb, ika_type_to_c(decl.type));
  str_builder_append(b->sb, " ");
  str_builder_append(b->sb, decl.symbol->symbol.value);
  if (decl.expr) {
    str_builder_append(b->sb, " = ");
    build_expr(b, decl.expr);
  }
  str_builder_append(b->sb, ";\n");
}

static void add_indent(c11_be_t *b) {
  for (int i = 0; i < b->ident_level; i++)
    str_builder_append(b->sb, "  ");
}

static void build_block(c11_be_t *b, ast_node_t *node) {
  ASSERT_MSG((node->type == ast_block), "Expected a block node");
  block_t block = node->block;
  str_builder_append(b->sb, "{\n");
  for (int i = 0; i < darray_len(block.nodes); i++) {
    add_indent(b);
    build_node(b, &block.nodes[i]);
    str_builder_append(b->sb, "\n");
  }
  if (block.return_statement) {
    add_indent(b);
    str_builder_append(b->sb, "return");
    if (block.return_statement->returns.expr) {
      str_builder_append(b->sb, " ");
      build_expr(b, block.return_statement->returns.expr);
    }
    str_builder_append(b->sb, ";\n");
  }
  str_builder_append(b->sb, "}\n");
}

static void build_if(c11_be_t *b, ast_node_t *node) {
  ASSERT_MSG((node->type == ast_if_stmt), "Expected an if_stmt node");
  if_t if_stmt = node->if_stmt;
  str_builder_append(b->sb, "if (");
  build_expr(b, if_stmt.expr);
  str_builder_append(b->sb, ") ");
  build_block(b, if_stmt.if_block);
  if (if_stmt.else_block) {
    str_builder_append(b->sb, " else ");
    build_block(b, if_stmt.else_block);
  }
  str_builder_append(b->sb, "\n");
}

static void build_function(c11_be_t *b, ast_node_t *node) {
  ASSERT_MSG((node->type == ast_fn), "Expected a function node");
  fn_t fn = node->fn;
  str_builder_append(b->sb, ika_type_to_c(fn.return_type));
  str_builder_append(b->sb, " I_");
  str_builder_append(b->sb, fn.symbol->symbol.value);
  str_builder_append(b->sb, "(");
  for (int i = 0; i < darray_len(fn.parameters); i++) {
    decl_t decl = fn.parameters[i].decl;
    str_builder_append(b->sb, ika_type_to_c(decl.type));
    str_builder_append(b->sb, " ");
    str_builder_append(b->sb, decl.symbol->symbol.value);
    if (i < darray_len(fn.parameters) - 1)
      str_builder_append(b->sb, ", ");
  }
  str_builder_append(b->sb, ") ");
  b->ident_level++;
  build_block(b, fn.block);
  b->ident_level--;
  str_builder_append(b->sb, "\n");
}

static void build_print(c11_be_t *b, ast_node_t *node) {
  ASSERT_MSG((node->type == ast_print_stmt), "Expected a print_stmt node");
  print_t prt = node->print_stmt;
  // TODO: Add support for types besides string
  str_builder_append(b->sb, "print(");
  build_expr(b, prt.expr);
  str_builder_append(b->sb, ");");
}

static void build_assignment(c11_be_t *b, ast_node_t *node) {
  ASSERT_MSG((node->type == ast_assignment), "Expected a assignment node");
  assignment_t assignment = node->assignment;
  str_builder_append(b->sb, assignment.symbol->symbol.value);
  str_builder_append(b->sb, "=");
  build_expr(b, assignment.expr);
  str_builder_append(b->sb, ";\n");
}

static void build_node(c11_be_t *b, ast_node_t *node) {
  switch (node->type) {
  case ast_block:
    build_block(b, node);
    break;
  case ast_if_stmt:
    build_if(b, node);
    break;
  case ast_fn:
    build_function(b, node);
    break;
  case ast_decl:
    build_decl(b, node);
    break;
  case ast_fn_call:
    build_fn_call(b, node);
    // Add statement terminator becuase build_fn_call doesn't as the
    // call may be part of an expression.
    str_builder_append(b->sb, ";");
    break;
  case ast_print_stmt:
    build_print(b, node);
    break;
  case ast_assignment:
    build_assignment(b, node);
    break;
  default:
    printf("Unsupported ast_node in c11 backend: %d\n", node->type);
    ASSERT_MSG((false), "Unsupported ast_node in c11 backend\n");
    break;
  }
}

static void build_entry_point(c11_be_t *b) {
  str_builder_append(b->sb, "\nint main(int argc, char **argv) {\n");
  str_builder_append(b->sb, "  I_main();\n}\n");
}

static void add_includes(c11_be_t *b) {
  str_builder_append(b->sb, "#include <stdio.h>\n");
  str_builder_append(b->sb, "#include <rt/str.h>\n");
  str_builder_append(b->sb, "#include <rt/print.h>\n\n");
}

static char *get_c_filename(c11_be_t *b) {
  char *new_filename = imust_alloc(1001);
  u32 len = strnlen(b->filename, 1000);
  u32 name_start, name_end = 0;
  for (u32 i = len - 1; i > 0; i--) {
    if (b->filename[i] == '.')
      name_end = i;
    if (b->filename[i] == '/' || b->filename[i] == '\\')
      name_start = i + 1;
  }
  if (name_start != 0 && name_end != 0) {
    sprintf(new_filename, "%.*s.c", name_end - name_start,
            &b->filename[name_start]);
    return new_filename;
  } else {
    ASSERT_MSG((FALSE),
               "Couldn't parse filename to create matching C filename");
  }
}

b8 c11_generate(compilation_unit_t *unit) {
  printf("Generating C code\n");
  str_builder_t *builder = str_builder_init();
  c11_be_t b = {.sb = builder, .ident_level = 0, .filename = unit->src_file};
  add_includes(&b);
  for (u64 i = 0; i < darray_len(unit->root->block.nodes); i++) {
    build_node(&b, &unit->root->block.nodes[i]);
  }
  build_entry_point(&b);
  str c_code = str_builder_to_alloced_str(builder);
  str_builder_cleanup(builder);
  FILE *c_output = fopen(get_c_filename(&b), "w");
  fprintf(c_output, "%s", str_to_cstr(c_code));
  return TRUE;
}
