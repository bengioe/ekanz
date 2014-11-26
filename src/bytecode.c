#include <Ekanz.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
  return p;
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

static void bc_byte(bcp bc, uint8_t b){
  *bc->data++ = b;}
static void bc_push(bcp bc, int64_t v, ek_type* t){ 
  *bc->data++ = PUSH; 
  *(int64_t*)bc->data = v; bc->data += 8;
  *(int64_t*)bc->data = t; bc->data += 8;}
static void bc_pushlocal(bcp bc, int32_t l){ 
  *bc->data++ = PUSH_LOCAL; *(int32_t*)bc->data = l; bc->data += 4; }
static void bc_pushglobal(bcp bc, int32_t l){ 
  *bc->data++ = PUSH_GLOBAL; *(int32_t*)bc->data = l; bc->data += 4; }
static void bc_popintolocal(bcp bc, int32_t l){ 
  *bc->data++ = POP_INTO_LOCAL; *(int32_t*)bc->data = l; bc->data += 4; }
static void bc_popintoglobal(bcp bc, int32_t l){ 
  *bc->data++ = POP_INTO_GLOBAL; *(int32_t*)bc->data = l; bc->data += 4; }
static void bc_popn(bcp bc, int32_t l){ 
  *bc->data++ = POPN; *(int32_t*)bc->data = l; bc->data += 4; }
static void bc_add(bcp bc){ 
  *bc->data++ = ADD;}
static void bc_lessthan(bcp bc){ 
  *bc->data++ = LESSTHAN;}
static void bc_call(bcp bc){ 
  *bc->data++ = CALL;}
static void bc_newframe(bcp bc){
  *bc->data++ = NEW_FRAME;}
static void bc_return(bcp bc, int32_t n){
  *bc->data++ = RETURN; *(int32_t*)bc->data = n; bc->data += 4;}
static void bc_getattr(bcp bc, char* s, int64_t l){
  *bc->data++ = GETATTR;
  *(char**)bc->data = s; bc->data += sizeof(char*);
  *(int32_t*)bc->data = l; bc->data += 4;}

static int64_t bc_genlabel(bcp bc){
  static int64_t n = 0;
  return n++;}
static int64_t bc_condbranch(bcp bc, int64_t truelabel, int64_t falselabel){
  *bc->data++ = CONDBRANCH;
  *(int32_t*)bc->data = truelabel; bc->data += 4;
  *(int32_t*)bc->data = 0; bc->data += 4;
  *(int32_t*)bc->data = falselabel; bc->data += 4;
  *(int32_t*)bc->data = 0; bc->data += 4;
  return (int64_t)bc->data - 4;} 
static int64_t bc_branch(bcp bc, int64_t label){
  *bc->data++ = BRANCH;
  *(int32_t*)bc->data = label; bc->data += 4;
  *(int32_t*)bc->data = 0; bc->data += 4;
  return (int64_t)bc->data - 4;} 
static int64_t bc_backwardsbranch(bcp bc, int64_t label){
  *bc->data++ = BRANCH;
  int32_t id = *((int32_t*)label-1);
  *(int32_t*)bc->data = id; bc->data += 4;
  *(int32_t*)bc->data = 0; bc->data += 4;
  return (int64_t)bc->data - 4;} 
static void bc_label(bcp bc, int64_t* label){
  *bc->data++ = LABEL;
  *(int32_t*)bc->data = *label; bc->data += 4;
  *label = (int64_t)(bc->data);}
static void bc_setmarker(bcp bc, int64_t marker, int64_t label){
  *(int32_t*)marker = (int32_t)label;}
static void bc_setcondmarker(bcp bc, int64_t marker, int64_t iflabel, int64_t elselabel){
  *(int32_t*)marker = (int32_t)elselabel;
  *(int32_t*)(marker-8) = (int32_t)iflabel;}
//////////////////////////


static bcp create_next_blob(bcp root){
  while (root->next != NULL){
    root = root->next;
  }
  bcp bc = malloc(sizeof(ek_bytecode));
  bc->size = 1024;
  bc->data = malloc(1024);
  bc->startptr = bc->data;
  bc->next = NULL;
  root->next = bc;
  return bc;
}

/////////////////

static void _write(int64_t arg, ek_type* argt, int64_t* rval, ek_type** rtype){
  if (argt == ek_IntType){
    printf("%ld\n",arg);
  }else if (argt == ek_StrType){
    ekop o = (ekop)arg;
    char* p = (char*)ek_obj_getextra(o,0);//*((char**)o->data + ek_StrType->nentries);
    unsigned int l = (unsigned int)ek_obj_getextra(o,1);//*((uint64_t*)o->data + ek_StrType->nentries + 1);
    printf("str: %.*s %p %p\n",l,p,o,arg);
  }else{
    printf("<%ld %p %p %p>\n", arg, argt, rval, rtype);
  }
  *rval = ek_None;
  *rtype = ek_NoneType;
}

static void load_builtin(bcp bc, scope_t* scope){
  char* names[] = {"__str__", "write"};
  ek_type* t = ek_type_deriveN(ek_NoneType, names, 2);
  
  ekop ek = ek_type_instanciate(t);

  ekop write = ek_type_instanciate(ek_CFuncType);
  ek_obj_setextra(write, 0, (ekop)_write);
  ek_obj_setattr_str(ek, "write", write);
  int64_t pos;
  pos = scope->frame_size++; scope_setn(scope, "ek", 2, pos);  bc_push(bc, ek, t);
  pos = scope->frame_size++; scope_setn(scope, "print", 5, pos); bc_push(bc, write, write->type);
}

///////////
/*
  find all the locals _declared_ in this AST root, and push them on the stack 
 */
static void find_and_push_locals(bcp bc, astnp root, scope_t* globals, scope_t* locals){
  if (root == NULL) return;
  switch(root->type){
  case EK_AST_BLOCK_NODE:
    find_and_push_locals(bc,root->left,globals, locals);
    find_and_push_locals(bc,root->right, globals, locals);
    break;
  case EK_AST_IF:
    find_and_push_locals(bc,root->right->left, globals, locals);
    find_and_push_locals(bc,root->right->right, globals, locals);
    break;
  case EK_AST_WHILE:
  case EK_AST_ELSE:
    find_and_push_locals(bc,root->right, globals, locals);
    break;    
  case EK_AST_ASSIGN:{
    astnp l = root->left;
    int64_t pos;
    if (!scope_get(locals, l, &pos)){// && !scope_get(globals, l, &pos)){
      pos = locals->frame_size++;
      scope_set(locals, l, pos);
      //printf("push None %p\n",ek_NoneType);
      bc_push(bc, 0, ek_NoneType);
      //printf("new scope variable %.*s(%ld)\n",l->toklen, l->tokstr, pos);
    }
    break;
  }
  case EK_AST_DEF:{
    astnp l = root->left->left;
    int64_t pos;
    if (!scope_get(locals, l, &pos)){// && !scope_get(locals, l, &pos)){
      pos = locals->frame_size++;
      scope_set(locals, l, pos);
      //printf("push None\n");
      bc_push(bc, 0, ek_NoneType);
      //printf("new scope variable %.*s(%ld)\n",l->toklen, l->tokstr, pos);
    }
    break;
  }
  case EK_AST_RETURN:
  case EK_AST_CALL:
    break;
  default:
    printf("Error, find and push locals unhandled type %d\n", root->type);
  }
}
///////////




static void bc_block(bcp bc, astnp node, scope_t* globals, scope_t* locals);
static void bc_expression(bcp bc, astnp node, scope_t* globals, scope_t* locals);
static void bc_function(bcp bc, astnp root, scope_t* globals, scope_t* locals);


static void bc_block(bcp bc, astnp node, scope_t* globals, scope_t* locals){
  while (node != NULL){
    astnp decl = node->left;
    switch(decl->type){
    case EK_AST_ASSIGN:
      { 
	astnp l = decl->left;
	int64_t posg = -1, posl = -1;
	if (!scope_get(locals, l, &posl) && !scope_get(globals, l, &posg)){/*
	  pos = globals->frame_size++;
	  scope_set(globals, l, pos);
	  printf("push None\n");
	  bc_pushl(bc, NONE_CST_VALUE);
	  printf("    new scope variable %.*s(%ld)\n",l->toklen, l->tokstr, pos);*/
	  printf(" Error, variable should have been found by find_and_push_locals(...)\n");
	}
	bc_expression(bc, decl->right, globals, locals);
	if (posl != -1){
	  bc_popintolocal(bc, posl);
	} else if (posg != -1){
	  bc_popintoglobal(bc, posl);
	}
	break;
      }
    case EK_AST_CALL:
      bc_expression(bc, decl, globals, locals);
      bc_popn(bc, 1); // pop the unused result of the call (remember
		      // we are at bc_block level, so the call looks
		      // like f(...) rather than x=f() so the return
		      // value that was pushed on the stack must be
		      // popped)
      
      break;
    case EK_AST_IF:{
      // evaluate the condition
      bc_expression(bc, decl->left, globals, locals);
      int64_t iflabel = bc_genlabel(bc);
      int64_t elselabel = bc_genlabel(bc);
      int64_t endlabel = bc_genlabel(bc);
      // branch to the else on False-ish
      int64_t ifelsemarker = bc_condbranch(bc, iflabel, elselabel);
      bc_label(bc, &iflabel);
      // execute the body on True-ish
      bc_block(bc, decl->right->left, globals, locals);
      int64_t endmarker = bc_branch(bc, endlabel);
      bc_label(bc, &elselabel);
      if (decl->right->right != NULL){
	bc_block(bc, decl->right->right->left, globals, locals);
      }
      int64_t elseendmarker = bc_branch(bc, endlabel);
      bc_label(bc, &endlabel);
      
      bc_setcondmarker(bc, ifelsemarker, iflabel, elselabel);
      bc_setmarker(bc, endmarker, endlabel);
      bc_setmarker(bc, elseendmarker, endlabel);
      break;
    }
    case EK_AST_WHILE:{
      int64_t startlabel = bc_genlabel(bc);
      int64_t blocklabel = bc_genlabel(bc);
      int64_t endlabel = bc_genlabel(bc);
      bc_label(bc, &startlabel);
      // evaluate the condition
      bc_expression(bc, decl->left, globals, locals);
      int64_t condmarker = bc_condbranch(bc, blocklabel, endlabel);
      bc_label(bc, &blocklabel);
      // execute the body on True-ish
      bc_block(bc, decl->right, globals, locals);
      int64_t startmarker = bc_backwardsbranch(bc, startlabel);
      bc_label(bc, &endlabel);
      bc_setcondmarker(bc, condmarker, blocklabel, endlabel);
      bc_setmarker(bc, startmarker, startlabel);
      break;
    }
    case EK_AST_DEF:{
      // create new code blob
      bcp defbc = create_next_blob(bc);
      scope_t* scope = scope_new();
      // set the arguments to be ordered in the scope
      // which are in a list of param
      astnp param = decl->left->right;
      while (param != NULL){
	int64_t pos = scope->frame_size++;
	scope_set(scope, param->left, pos);
	param = param->right;
      }
      bc_function(defbc, decl->right, globals, scope);
      astnp l = decl->left->left;
      int64_t posg = -1, posl = -1;
      if (!scope_get(locals, l, &posl) && !scope_get(globals, l, &posg)){
	printf(" Error, variable should have been found by find_and_push_locals(...)\n");
      }
      bc_push(bc, (int64_t)defbc->startptr, ek_FuncType);
      if (posl != -1){
	bc_popintolocal(bc, posl);
      } else if (posg != -1){
	bc_popintoglobal(bc, posg);
      }
      break;
    }
    case EK_AST_RETURN:
      bc_expression(bc, decl->left, globals, locals);
      bc_return(bc, locals->frame_size); break;
    default:
      bc_expression(bc, decl, globals, locals);
      //printf("Error, Unknown AST node %d (from bc_block)\n",decl->type);
    }
    node = node->right;
  }
}

static void bc_expression(bcp bc, astnp node, scope_t* globals, scope_t* locals){
  switch(node->type){
  case EK_AST_CSTINT:{
    int64_t v = atoi(node->tokstr);
    
    bc_push(bc, v, ek_IntType);
    //printf("push %.*s (%d)\n", node->toklen, node->tokstr, v);
    break;
  }
  case EK_AST_CSTSTR:{
    // strings are immutable, so it should be ok to create an object
    // once and use it for the rest of the execution
    ekop v = ek_type_instanciate(ek_StrType);
    char** sp = (char**)v->data + ek_StrType->nentries;
    unsigned long* sl = (unsigned long*)v->data + ek_StrType->nentries + 1;
    *sp = node->tokstr;
    *sl = node->toklen;
    printf("strdef: %.*s %p\n",*sl,*sp,v);
    bc_push(bc, v, ek_StrType);
    break;
  }
  case EK_AST_VARNAME:{
    int64_t posg = -1, posl = -1;
    if (!scope_get(locals, node, &posl) && !scope_get(globals, node, &posg)){
      printf("NameError, %.*s is not defined\n", node->toklen, node->tokstr);
    }
    if (posl != -1){
      if (posl < 0)
	printf("WAT\n");
	//bc_push(bc, -posl, ek_CFuncType);
      else
	bc_pushlocal(bc, posl);
    }else if(posg != -1){
      if (posg < 0)
	printf("WATX\n");
      //bc_push(bc, -posg, ek_CFuncType);
      else
	bc_pushglobal(bc, posg);
    }
    //printf("push from %ld\n",posx);
    break;
  }

  case EK_AST_ADD:
    bc_expression(bc, node->left, globals, locals); bc_expression(bc, node->right, globals, locals);
    bc_add(bc); break;
  case EK_AST_SUB:
    bc_expression(bc, node->left, globals, locals); bc_expression(bc, node->right, globals, locals);
    bc_byte(bc, SUB); break;
  case EK_AST_MUL:
    bc_expression(bc, node->left, globals, locals); bc_expression(bc, node->right, globals, locals);
    bc_byte(bc, MUL); break;
  case EK_AST_DIV:
    bc_expression(bc, node->left, globals, locals); bc_expression(bc, node->right, globals, locals);
    bc_byte(bc, DIV); break;

  case EK_AST_LESS:
    bc_expression(bc, node->left, globals, locals); bc_expression(bc, node->right, globals, locals);
    bc_lessthan(bc); break;

  case EK_AST_CALL:
    // evaluate arguments
    // todo: allow calling a function with multiple arguments
    bc_expression(bc, node->right, globals, locals);
    // then function object
    bc_expression(bc, node->left, globals, locals);
    // call it
    bc_call(bc);
    //printf("call\n");
    break;
  case EK_AST_DOT:{
    // evaluate left side 
    bc_expression(bc, node->left, globals, locals);
    // getattr right side
    bc_getattr(bc, node->right->tokstr, node->right->toklen);
    break;
  }
  default:
    printf("\n\nError in bc_expression unknown node type (%d)\n\n\n",node->type);
  }
}

static void bc_function(bcp bc, astnp root, scope_t* globals, scope_t* locals){
  int64_t start = bc_genlabel(bc);
  bc_label(bc, &start);
  bc_newframe(bc);
  find_and_push_locals(bc, root, globals, locals);
  bc_block(bc, root, globals, locals);
  //bc_popn(bc, locals->frame_size);
  bc_return(bc, locals->frame_size);
}

/*
void native_print(int64_t arg,ek_type* argt,int64_t* x,ek_type** y){
  if (argt == ek_IntType){
    printf("%ld\n",arg);
  }else if (argt == ek_StrType){
    ekop o = (ekop)arg;
    char* p = *((char**)o->data + ek_StrType->nentries);
    uint64_t l = *((uint64_t*)o->data + ek_StrType->nentries + 1);
    printf("%.*s\n",l,p);
  }else{
    printf("<%ld %p %p %p\n", arg, argt, x, y);
  }
  *x = ek_None;
  *y = ek_NoneType;
  }*/

ek_bytecode* ek_bc_compile_ast(astnp root){
  bcp bc = malloc(sizeof(ek_bytecode));
  bc->size = 1024;
  bc->data = malloc(1024);
  bc->startptr = bc->data;
  bc->next = NULL;
  ek_basicblock* bb = malloc(sizeof(ek_basicblock));
  bb->bcstart = bc->data;
  bb->bbv = NULL;
  bc->startbb = bb;
  bc->bb = bb;

  scope_t* globals = scope_new();
  int64_t start = bc_genlabel(bc);
  bc_label(bc, &start);
  bc_newframe(bc);

  load_builtin(bc, globals);

  find_and_push_locals(bc, root, globals, globals);
  bc_block(bc, root, globals, globals);
  //printf("popn %d\n", globals->frame_size);
  //bc_popn(bc, globals->frame_size);
  //printf("ret\n");
  bc_return(bc, globals->frame_size);
  return bc;
}




void ek_bc_print(ek_bytecode* bc){
  char* p = bc->startptr;
  while (*p){
    printf("%p:   ",p);
    switch(*p++){
    case PUSH:
      printf("push %p %p\n",*(int64_t*)p,*(int64_t*)(p+8)); p+=16; break;
    case POP:
      printf("pop\n"); break;
    case POPN:
      printf("popn %d\n",*(int*)p); p+=4; break;
    case PUSH_LOCAL:
      printf("push local %d\n",*(int*)p); p+=4; break;
    case PUSH_GLOBAL:
      printf("push global %d\n",*(int*)p); p+=4; break;
    case POP_INTO_LOCAL:
      printf("pop into local %d\n",*(int*)p); p+=4; break;
    case POP_INTO_GLOBAL:
      printf("pop into global %d\n",*(int*)p); p+=4; break;
    case ADD:
      printf("add\n"); break;
    case SUB:
      printf("sub\n"); break;
    case MUL:
      printf("mul\n"); break;
    case DIV:
      printf("DIV\n"); break;
    case LESSTHAN:
      printf("lessthan\n"); break;
    case CALL:
      printf("call\n"); break;
    case RETURN:
      printf("return %d\n",*(int*)p); p+=4; break;
    case NEW_FRAME:
      printf("new_frame\n"); break;
    case LABEL:
      printf("\r                     \r <%d>:\n", *(int*)p); p+=4; break;
    case BRANCH:
      printf("branch <%d>\n", *(int*)p); p+=8; break;
    case CONDBRANCH:
      printf("condbranch <%d>:<%d> \n", *(int*)p, *(int*)(p+8));
      p+=16; break;
    case GETATTR:
      printf("getattr '%.*s'\n",*(int*)(p+8),  *(char**)p);
      p+=12; break;
    default:
      printf("unknown? %d\n",*(p-1));
    }
  }
  printf("next? %p\n",bc->next);
  if (bc->next){
    ek_bc_print(bc->next);
  }
}
