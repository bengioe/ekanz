#ifndef EK_BYTECODE_H
#define EK_BYTECODE_H

#include "parse.h"


enum ByteCodeOps {
  PUSH = 32,
  POP,
  POPN,
  PUSH_FROM,
  POP_INTO,
  ADD,
  CALL,
  RETURN,
  NEW_FRAME,
  LABEL,
  BRANCH,
  CONDBRANCH,
};

#define NONE_CST_VALUE 42

typedef struct ek_bytecode{
  char* data;
  int size;
  char* startptr;
} ek_bytecode;


ek_bytecode* ek_bc_compile_ast(ek_ast_node* ast);
void ek_bc_print(ek_bytecode*);

#endif
