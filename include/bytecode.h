#ifndef EK_BYTECODE_H
#define EK_BYTECODE_H

#include "parse.h"


enum ByteCodeOps {
  PUSH = 32,
  POP,
  POPN,
  PUSH_LOCAL,
  PUSH_GLOBAL,
  POP_INTO_LOCAL,
  POP_INTO_GLOBAL,
  ADD,
  SUB, // 40
  DIV,
  MUL,
  LESSTHAN, 
  CALL,
  RETURN, 
  NEW_FRAME, 
  LABEL,
  BRANCH,
  CONDBRANCH,
  GETATTR, // 50
};

#define NONE_CST_VALUE 42
#define UNDEFINED_CST_VALUE 43

typedef struct ek_bblockversion{
  int nassumptions;
  int* types;
  void* code;
} ek_bblockversion;

typedef struct ek_basicblock{
  char* bcstart; // pointer to start of bytecode
  char* bcend;
  ek_bblockversion* bbv;
} ek_basicblock;

typedef struct ek_bytecode{
  char* data;
  int size;
  char* startptr;
  struct ek_bytecode* next;
  ek_basicblock* startbb;
  ek_basicblock* bb;
} ek_bytecode;


ek_bytecode* ek_bc_compile_ast(ek_ast_node* ast);
void ek_bc_print(ek_bytecode*);

#endif
