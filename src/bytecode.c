#include <Ekanz.h>

#include <stdlib.h>
#include <stdio.h>

#define astnp ek_ast_node*
#define bcp ek_bytecode*

// linked list with sentinel
typedef struct strdict_node{
  char* key;
  int64_t value;
  struct strdict_node* next;
} strdict_node;

static strdict_node* strdict_new(){
  strdict_node* p = malloc(sizeof(strdict_node));
  p->key = NULL;
  p->value = 0;
  p->next = NULL;
}

static int64_t strdict_getn(strdict_node* d, char* k, int64_t len, int64_t* to){
  while (d->next != NULL && strncmp(d->key, k, len) != 0){
    d = d->next;
  }
  if (d->next){
    *to = d->value;
    return 1;
  }
  return 0;
}

static void strdict_setn(strdict_node* d, char* k, int64_t len, int64_t v){
  strdict_node* root = d;
  while (d->next != NULL && strncmp(d->key, k, len) != 0){
    d = d->next;
  }
  if (d->next == NULL){
    d->next = strdict_new();
    d->key = k;
  }
  d->value = v;
}

//////////////////////////

static void bc_pushl(bcp bc, int64_t l){ 
  *bc->data++ = PUSH; *(int64_t*)bc->data = l; bc->data += 8; }
static void bc_pushfrom(bcp bc, int64_t l){ 
  *bc->data++ = PUSH_FROM; *(int64_t*)bc->data = l; bc->data += 8; }
static void bc_popinto(bcp bc, int64_t l){ 
  *bc->data++ = POP_INTO; *(int64_t*)bc->data = l; bc->data += 8; }
static void bc_add(bcp bc){ 
  *bc->data++ = ADD;}
static void bc_call(bcp bc){ 
  *bc->data++ = CALL;}
static void bc_return(bcp bc){
  *bc->data++ = RETURN;}



//////////////////////////



static void bc_block(bcp bc, astnp node, strdict_node* globals);
static void bc_expression(bcp bc, astnp node, strdict_node* globals);

static void bc_block(bcp bc, astnp node, strdict_node* globals){
  int64_t local_scope_stack_ptr = 0;
  int64_t stack_size = 0;
  while (node != NULL){
    astnp decl = node->left;
    switch(decl->type){
    case EK_AST_ASSIGN:
      { 
	astnp l = decl->left;
	int64_t pos;
	if (!scopechain_get(scopes, l, &pos)){
	  pos = local_scope_stack_ptr++;
	  stack_size++;
	  scopechain_set(scopes, l, pos);
	  printf("push None\n");
	  bc_pushl(bc, NONE_CST_VALUE);
	  printf("    new scope variable %.*s(%ld)\n",l->toklen, l->tokstr, pos);
	}
	bc_expression(bc, decl->right, scopes);
	bc_popinto(bc, pos + stack_size);
	stack_size--;
	printf("pop into %ld\n", pos);
	break;
      }
    case EK_AST_CALL:
      bc_expression(bc, decl, scopes);
      break;
    default:
      printf("Error, Unknown AST node %d\n",node->type);
    }
    node = node->right;
  }
  bc_return(bc);
  scopechain_popscope(scopes);
}

static void bc_expression(bcp bc, astnp node, scopechain_t* scopes){
  switch(node->type){
  case EK_AST_CSTINT:{
    int64_t v = atoi(node->tokstr);
    bc_pushl(bc, v);
    printf("push %.*s (%d)\n", node->toklen, node->tokstr, v);
    break;
  }
  case EK_AST_VARNAME:{
    int64_t posx = -1;
    if (!scopechain_get(scopes, node, &posx)){
      printf("NameError, %.*s is not defined\n", node->toklen, node->tokstr);
    }
    bc_pushfrom(bc, posx);
    printf("push from %ld\n",posx);
    break;
  }
  case EK_AST_ADD:
    bc_expression(bc, node->left, scopes);
    bc_expression(bc, node->right, scopes);
    bc_add(bc);
    printf("add\n");
    break;
  case EK_AST_CALL:
    bc_expression(bc, node->left, scopes);
    bc_expression(bc, node->right, scopes);
    bc_call(bc);
    printf("call\n");
    break;
  default:
    printf("Error in bc_expression (%d)\n",node->type);
  }
}

void native_print(int64_t arg){
  printf("%ld\n", arg);
}

ek_bytecode* ek_bc_compile_ast(astnp root){
  bcp bc = malloc(sizeof(ek_bytecode));
  bc->size = 1024;
  bc->data = malloc(1024);
  bc->startptr = bc->data;
  scopechain_t* scopes = scopechain_new();
  scopes = scopechain_pushscope(scopes);
  scopechain_setn(scopes, "print", 5, -(int64_t)&native_print);
  bc_block(bc, root, scopes);
  return bc;
}

