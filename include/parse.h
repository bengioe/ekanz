#ifndef EK_PARSE_H
#define EK_PARSE_H

#define EK_AST_CSTSTR 256
#define EK_AST_CSTINT 257
#define EK_AST_VARNAME 258
#define EK_AST_NUMBER 259  // why is there both number and int?
#define EK_AST_CALL 272
#define EK_AST_ADD 273
#define EK_AST_DOT 274
#define EK_AST_IF 275
#define EK_AST_IFNODE 276
#define EK_AST_ELSE 277
#define EK_AST_ELIF 278
#define EK_AST_ASSIGN 279
#define EK_AST_BLOCK_NODE 280

typedef struct ek_ast_node{
  char* tokstr;
  int32_t toklen;
  int32_t type;
  struct ek_ast_node* left;
  struct ek_ast_node* right;
} ek_ast_node;

ek_ast_node* ek_parse_text(char*, char*);
void ek_parse_print_ast(ek_ast_node*);

#endif
