#ifndef EK_PARSE_H
#define EK_PARSE_H

#include "obj.h"

enum AstNodeTypes{

  EK_AST_CSTSTR=256,
  EK_AST_CSTINT,
  EK_AST_VARNAME,
  EK_AST_NUMBER, // why is there both number and int?
  EK_AST_CALL, // 260
  EK_AST_GETITEM,
  EK_AST_ADD,
  EK_AST_SUB,
  EK_AST_LESS,
  EK_AST_MUL,
  EK_AST_DIV,
  EK_AST_DOT,
  EK_AST_IF,
  EK_AST_IFNODE,
  EK_AST_ELSE, // 270
  EK_AST_ELIF,
  EK_AST_ASSIGN,
  EK_AST_BLOCK_NODE,
  EK_AST_DEF,
  EK_AST_RETURN,
  EK_AST_WHILE,
  EK_AST_PARAM,
};

typedef struct ek_ast_node{
  char* tokstr;
  int32_t toklen;
  int32_t type;
  struct ek_ast_node* left;
  struct ek_ast_node* right;
  uint16_t local_scope_size;
  uint8_t isCompiledUnit;
  union x{
    struct var{
      uint32_t scoperef; // scope reference id of this node, should be
			 // unique across all scopes (for the same
			 // variable)
      int32_t scopepos; // position on the local stack (negative is for global scope)
      ek_type* predicted_type;
    } var;
    struct jit{
      uint8_t isLeafUnit; // a node that has no BLOCK_NODE children
      char* block;
    } jit;
  } x;
} ek_ast_node;

ek_ast_node* ek_parse_text(char*, char*);
void ek_parse_print_ast(ek_ast_node*);

#endif
