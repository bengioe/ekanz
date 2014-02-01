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

typedef struct scope_t {
  strdict_node* dict;
  int64_t frame_size;
} scope_t;

static scope_t* scope_new(){
  scope_t* s = malloc(sizeof(scope_t));
  s->dict = strdict_new();
  s->frame_size = 0;
  return s;
}

static int64_t scope_get(scope_t* s, astnp n, int64_t* to){
  return strdict_getn(s->dict, n->tokstr, n->toklen, to);
}

static void scope_set(scope_t* s, astnp n, int64_t v){
  return strdict_setn(s->dict, n->tokstr, n->toklen, v);
}

static void scope_setn(scope_t* s, char* k, int len, int64_t v){
  return strdict_setn(s->dict, k, len, v);
}

//////////////////////////

static void bc_pushl(bcp bc, int32_t l){ 
  *bc->data++ = PUSH; *(int64_t*)bc->data = l; bc->data += 4; }
static void bc_pushfrom(bcp bc, int32_t l){ 
  *bc->data++ = PUSH_FROM; *(int32_t*)bc->data = l; bc->data += 4; }
static void bc_popinto(bcp bc, int32_t l){ 
  *bc->data++ = POP_INTO; *(int32_t*)bc->data = l; bc->data += 4; }
static void bc_popn(bcp bc, int32_t l){ 
  *bc->data++ = POPN; *(int32_t*)bc->data = l; bc->data += 4; }
static void bc_add(bcp bc){ 
  *bc->data++ = ADD;}
static void bc_call(bcp bc){ 
  *bc->data++ = CALL;}
static void bc_newframe(bcp bc){
  *bc->data++ = NEW_FRAME;}
static void bc_return(bcp bc){
  *bc->data++ = RETURN;}

//////////////////////////



static void bc_block(bcp bc, astnp node, scope_t* globals);
static void bc_expression(bcp bc, astnp node, scope_t* globals);

static void bc_block(bcp bc, astnp node, scope_t* globals){
  while (node != NULL){
    astnp decl = node->left;
    switch(decl->type){
    case EK_AST_ASSIGN:
      { 
	astnp l = decl->left;
	int64_t pos;
	if (!scope_get(globals, l, &pos)){
	  pos = globals->frame_size++;
	  scope_set(globals, l, pos);
	  printf("push None\n");
	  bc_pushl(bc, NONE_CST_VALUE);
	  printf("    new scope variable %.*s(%ld)\n",l->toklen, l->tokstr, pos);
	}
	bc_expression(bc, decl->right, globals);
	bc_popinto(bc, pos);
	printf("pop into %ld\n", pos);
	break;
      }
    case EK_AST_CALL:
      bc_expression(bc, decl, globals);
      break;
    default:
      printf("Error, Unknown AST node %d\n",node->type);
    }
    node = node->right;
  }
  printf("popn %d\n", globals->frame_size);
  bc_popn(bc, globals->frame_size);
  printf("ret\n");
  bc_return(bc);
}

static void bc_expression(bcp bc, astnp node, scope_t* globals){
  switch(node->type){
  case EK_AST_CSTINT:{
    int64_t v = atoi(node->tokstr);
    bc_pushl(bc, v);
    printf("push %.*s (%d)\n", node->toklen, node->tokstr, v);
    break;
  }
  case EK_AST_VARNAME:{
    int64_t posx = -1;
    if (!scope_get(globals, node, &posx)){
      printf("NameError, %.*s is not defined\n", node->toklen, node->tokstr);
    }
    bc_pushfrom(bc, posx);
    printf("push from %ld\n",posx);
    break;
  }
  case EK_AST_ADD:
    bc_expression(bc, node->left, globals);
    bc_expression(bc, node->right, globals);
    bc_add(bc);
    printf("add\n");
    break;
  case EK_AST_CALL:
    bc_expression(bc, node->left, globals);
    bc_expression(bc, node->right, globals);
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
  scope_t* globals = scope_new();
  scope_setn(globals, "print", 5, -(int64_t)&native_print);
  bc_newframe(bc);
  bc_block(bc, root, globals);
  return bc;
}

