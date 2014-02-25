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
static int64_t bc_genlabel(bcp bc){
  static int64_t n = 0;
  return n++;}
static int64_t bc_condbranch(bcp bc, int64_t label){
  *bc->data++ = CONDBRANCH;
  *(int32_t*)bc->data = label; bc->data += 4;
  *(int32_t*)bc->data = 0; bc->data += 4;
  return (int64_t)bc->data - 4;} 
static int64_t bc_branch(bcp bc, int64_t label){
  *bc->data++ = BRANCH;
  *(int32_t*)bc->data = label; bc->data += 4;
  *(int32_t*)bc->data = 0; bc->data += 4;
  return (int64_t)bc->data - 4;} 
static int64_t bc_label(bcp bc, int64_t* label){
  *bc->data++ = LABEL;
  *(int32_t*)bc->data = *label; bc->data += 4;
  *label = (int64_t)(bc->data);}
static int64_t bc_setmarker(bcp bc, int64_t marker, int64_t label){
  *(int32_t*)marker = (int32_t)label;}

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
    case EK_AST_IF:
      // evaluate the condition
      bc_expression(bc, decl->left, globals);
      int64_t elselabel = bc_genlabel(bc);
      int64_t endlabel = bc_genlabel(bc);
      // branch to the else on False-ish
      int64_t elsemarker = bc_condbranch(bc, elselabel);
      // execute the body on True-ish
      bc_block(bc, decl->right->left, globals);
      int64_t endmarker = bc_branch(bc, endlabel);
      bc_label(bc, &elselabel);
      if (decl->right->right != NULL){
	printf("DRR\n");
	ek_parse_print_ast(decl->right->right);
	bc_block(bc, decl->right->right->left, globals);
      }
      bc_label(bc, &endlabel);
      
      bc_setmarker(bc, elsemarker, elselabel);
      bc_setmarker(bc, endmarker, endlabel);
      break;
    default:
      printf("Error, Unknown AST node %d (from bc_block)\n",decl->type);
    }
    printf("Passed to right\n");
    node = node->right;
  }
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
  int64_t start = bc_genlabel(bc);
  bc_label(bc, &start);
  bc_newframe(bc);
  bc_block(bc, root, globals);
  printf("popn %d\n", globals->frame_size);
  bc_popn(bc, globals->frame_size);
  printf("ret\n");
  bc_return(bc);
  return bc;
}




void ek_bc_print(ek_bytecode* bc){
  char* p = bc->startptr;
  while (*p){
    printf("%p:   ",p);
    switch(*p++){
    case PUSH:
      printf("push %d\n",*(int*)p); p+=4; break;
    case POP:
      printf("pop\n"); break;
    case POPN:
      printf("popn %d\n",*(int*)p); p+=4; break;
    case PUSH_FROM:
      printf("push from %d\n",*(int*)p); p+=4; break;
    case POP_INTO:
      printf("pop into %d\n",*(int*)p); p+=4; break;
    case ADD:
      printf("add\n"); break;
    case CALL:
      printf("call\n"); break;
    case RETURN:
      printf("return\n"); break;
    case NEW_FRAME:
      printf("new_frame\n"); break;
    case LABEL:
      printf("\r                     \r <%d>:\n", *(int*)p); p+=4; break;
    case BRANCH:
      printf("branch <%d> \t%p\n", *(int*)p, *(int*)(p+4)); p+=8; break;
    case CONDBRANCH:
      printf("condbranch <%d> \t%p\n", *(int*)p, *(int*)(p+4)); p+=8; break;
    default:
      printf("unknown? %d\n",*(p-1));
    }
  }
}
