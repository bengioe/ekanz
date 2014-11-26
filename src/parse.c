#include <Ekanz.h>

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>

#define nodep ek_ast_node* 

typedef struct parse_state{
  char* strptr;
  char* strstart;
  char* filename;
  int current_indent;
  int used_indent;
} parse_state;

// generic print
static void printnodep(nodep n){
  printf("<ek_ast_node* %.*s %d %p %p>", n->toklen, n->tokstr, n->type, n->left, n->right);
}

// empty nodep
static nodep new_nodep(){
  nodep n = (nodep)malloc(sizeof(ek_ast_node));
  n->tokstr = NULL;
  n->left = NULL;
  n->right = NULL;
  return n;
}

// nodep with left and right children
static nodep new_nodeplr(nodep l, nodep r){
  nodep n = (nodep)malloc(sizeof(ek_ast_node));
  n->tokstr = NULL;
  n->left = l;
  n->right = r;
  return n;
}

// nodep with left and right children and type
static nodep new_nodeplrt(nodep l, nodep r, int type){
  nodep n = (nodep)malloc(sizeof(ek_ast_node));
  n->tokstr = NULL;
  n->left = l;
  n->right = r;
  n->type = type;
  return n;
}


static void free_nodep(nodep node){
  
}


static int line_length(char* s){
  int n = 0;
  while (*s++ != '\n' && *s != 0){
    n++;
  }
  return n;
}

static char* line_start(char* s){
  while (*s != '\n' && *s != 0){
    s--;
  }
  return s+1;
}

static int line_count(char* s, char* end){
  int count = 1;
  while (s < end){
    if (*s++ == '\n')
      count++;
  }
  return count;
}

static jmp_buf error_buf;

static void parse_error(parse_state* S, char* message){
  printf("  File \"%s\", line %d\n", S->filename, line_count(S->strstart, S->strptr));
  printf("SyntaxError: %s\n", message);
  char* lstart = line_start(S->strptr);
  printf("%.*s\n", line_length(lstart), lstart, line_length(lstart));
  longjmp(error_buf, 1);
}

static int p_INDENT(parse_state* S){
  int count = 0;
  while (*S->strptr == ' '){
    S->strptr++;
    count++;
  }
  if (count == 0){
    return 0;
  }
  if (S->used_indent == -1){
    S->used_indent = count;
    if (count != 2 && count != 4){
      printf("Warning: irregular indentation (%d spaces)\n", count);
    }
  }
  //printf("indent %d / %d\n",count, S->used_indent);
  if (count % S->used_indent != 0){
    return -1;
  }
  return count / S->used_indent;
}

// skip trailing whitespace
static void P_skipWS(parse_state* S){
  while (*S->strptr == ' '){
    S->strptr++;
  }
  //printf("Skipped to %c\n", *S->strptr);
}

// skip trailing whitespace
static void P_skipWSNL(parse_state* S){
  while (*S->strptr == ' ' || *S->strptr == '\n'){
    S->strptr++;
  }
  //printf("Skipped to %c\n", *S->strptr);
}

static int P_see(parse_state* S, int type){
  P_skipWS(S);
  char* s = S->strptr;
  if (type < 256){
    if (*s == type){
      return 1;
    }
  } else if (type == EK_AST_VARNAME){
    char c = *s;
    //printf("seevalid_idstart: %c %d\n",c,c);
    if ( (c <= 'z' && c>= 'a') ||
	 (c <= 'Z' && c>= 'A')){
      return 1;
    }
  } else if (type == EK_AST_NUMBER){
    char c = *s;
    //printf("seevalid_number %c %d\n",c,c);
    if ( (c <= '9' && c>= '0') ){
      return 1;
    }
  } else if (type == EK_AST_IF){
    if (*s == 'i' && *(s+1)=='f' && *(s+2)==' '){
      return 2;
    }
  } else if (type == EK_AST_ELSE){
    if (*s == 'e' && *(s+1)=='l' && *(s+2)=='s' && *(s+3)=='e' && (*(s+4)==' ' || *(s+4)==':')){
      return 4;
    }
  } else if (type == EK_AST_ELIF){
    if (*s == 'e' && *(s+1)=='l' && *(s+2)=='i' && *(s+3)=='f' && *(s+4)==' '){
      return 4;
    }
  } else if (type == EK_AST_DEF){
    if (*s == 'd' && *(s+1)=='e' && *(s+2)=='f' && *(s+3)==' '){
      return 3;
    }
  } else if (type == EK_AST_WHILE){
    if (*s == 'w' && *(s+1)=='h' && *(s+2)=='i' && *(s+3)=='l' && *(s+4)=='e' && *(s+5)==' '){
      return 5;
    }
  } else if (type == EK_AST_RETURN){
    if (*s == 'r' && *(s+1)=='e' && *(s+2)=='t' && *(s+3)=='u' && *(s+4)=='r' && *(s+5)=='n' && *(s+6)==' '){
      return 6;
    }
  } 
  return 0;
}

static int P_accept(parse_state* S, int type){
  int length;
  if ((length = P_see(S, type)) != 0){
    S->strptr += length;
    return length;
  }
  return 0;
}

static int P_seevalid_id(parse_state* S){
  char c = *S->strptr;
  if ( (c <= 'z' && c>= 'a') ||
       (c <= 'Z' && c>= 'A') ||
       (c <= '9' && c>= '0')){
    return 1;
  }
  return 0;
}

static void P_rewind(parse_state* S, int len){
  S->strptr -= len;
}


const char* ek_ast_typenames[] = {
  // first is 256 - 8 = 248
  "invalid",0,0,0, 0,0,0,0, // next is 8(256)
  "str",
  "int",
  "varname",
  "number",
  "call", // 260
  "getitem",
  "+",
  "-",
  "<",
  "*",
  "/",
  ".",
  "if",
  "ifnode",
  "else", // 270
  "elif",
  "assign", 
  "block_element", 
  "def",
  "return",
  "while",
  "argument",
  "invalid_",
};


static void _astp(nodep n, int level){
  int i = level;
  while (i-- > 0){
    printf(" ");
  }
  if (n == NULL){
    printf("(nil?)\n");
    return;
  }
  if (n->type >= 256){
    printf("| %s", ek_ast_typenames[n->type-248]);
  }else{
    printf("| %c",n->type);
  }
  if (n->tokstr){
    printf(" '%.*s'",n->toklen, n->tokstr);
  }
  printf("\n");
  if (n->type != EK_AST_BLOCK_NODE) level++;
  if (n->left){
    _astp(n->left, level);
  }
  if (n->right){
    _astp(n->right, level);
  }
}

void ek_parse_print_ast(nodep root){
  printf("AST %p:\n",root);
  _astp(root, 0);
}


/**
   The grammar for Ekanz is implicitly inscribed in this code, and
   explicitely (as much as possible) right here: */


static nodep p_block(parse_state*);
/* block: declaration **/
static nodep p_declaration(parse_state*);
/* declaration : funcdef | ifstmt | assignment | whilestmt | retstmt*/
static nodep p_funcdef(parse_state*);
/* funcdef : 'def' VARIABLE parameters ':' block */
static nodep p_parameters(parse_state*);
/* parameters: VARIABLE (, VARIABLE)*  */
static nodep p_whilestmt(parse_state*);
/* whilestmt: 'while expression : '\n' block */
static nodep p_ifstmt(parse_state*);
/* ifstmt : ('if' expression : '\n' block)
          | ('if' expression : '\n' block 
	     (('elif'|'else' 'if') expression: '\n' block)* 
	     (else: '\n' block)?) */
static nodep p_retstmt(parse_state*);
/* retstmt : 'return' expression */
static nodep p_assignment(parse_state*);
/* assignment : expression/A '=' expression | expression
   assert !isLitteral(A)
 */
static nodep p_expression(parse_state*);
/* expression : addition */
static nodep p_addition(parse_state*);
/* addition : multiplication ('+' | '-') addition | power*/
static nodep p_multiplication(parse_state*);
/* multiplication : power (('*' | '/') power)*    
   -- note the ((m|d) p)* required for left assoc  */
static nodep p_power(parse_state*);
/* power: value trailer*    */
static nodep p_trailer(parse_state*);
/* trailer :  '(' expression ')' | '.' VARIABLE */
static nodep p_value(parse_state*);
/* value : VARIABLE | NUM_CST | STR_CST */
static nodep p_VARIABLE(parse_state*);


nodep ek_parse_text(char* text, char* filename){

  parse_state S;
  S.strptr = text;
  S.strstart = text;
  S.current_indent = -1;
  S.filename = filename;
  /* The first indentation size found determines what is used for the
     rest of the file, thus it starts at an unknown value, -1.
   */
  S.used_indent = -1;
  if (setjmp(error_buf)){
    return NULL;
  }else{
    nodep root = p_block(&S);
    return root;
  }

  return NULL;
}


static nodep p_block(parse_state* S){
  nodep root = NULL;
  nodep temp; nodep last; nodep line;
  int block_indent = ++S->current_indent;
  do{
    //printf("block with indent %d\n", block_indent);
    //printf("%.*s\n",line_length(S->strptr),S->strptr);
    int indent = p_INDENT(S);
    if (indent != block_indent){
      if (indent >= 0 && indent < block_indent){
	// we saw an indentation token, but it didn't belong to this
	// block, so we rewind and return
	P_rewind(S, indent * S->used_indent);
	break;
      }
      parse_error(S, "Unexpected indentation!");
    }
    line = p_declaration(S);
    if (line == NULL){
      if (P_accept(S, '\n')){
	line = 1;
	continue;
      }
      if (P_accept(S, '#')){
	while (*S->strptr != '\n'){
	  S->strptr++;
	}
	S->strptr++;
	line = 1;
	continue;
      }
      break;
    }
    temp = new_nodeplrt(line, NULL, EK_AST_BLOCK_NODE);
    //ek_parse_print_ast(temp);
    if (root == NULL){
      root = temp;
      last = temp;
    }else{
      last->right = temp;
      last = temp;
    }
  }while (line != NULL);
  S->current_indent--;
  return root;
}

static nodep p_declaration(parse_state* S){
  nodep root;
  //printf("declaration: %.*s\n",line_length(S->strptr), S->strptr);
  if ((root = p_funcdef(S)) != NULL){
    return root;
  }
  else if ((root = p_ifstmt(S)) != NULL){
    return root;
  }
  else if ((root = p_whilestmt(S)) != NULL){
    return root;
  }
  else if ((root = p_retstmt(S)) != NULL){
    return root;
  }
  else if ((root = p_assignment(S)) != NULL){
    //printf("Got expression()!\n");
    if (!P_accept(S, '\n')){
      printf("Error, no newline found...?\n");
    }
    //printf("Now at '%c' %d\n",*S->strptr,*S->strptr);
    return root;
  }
  return NULL;
}

static nodep p_funcdef(parse_state* S){
  /* 
     def nodes are structured as follows
     DEF L-> DEF L-> name
                 R-> parameters
	 R-> block
  */
  nodep root = NULL;
  if (P_accept(S, EK_AST_DEF)){
    nodep name = p_VARIABLE(S);
    if (!name){
      parse_error(S, "Expected name after 'def'");
    }
    if (!P_accept(S, '(')){
      parse_error(S, "Expected parenthesis after 'def ...'");
    }
    nodep args = p_parameters(S);
    if (!P_accept(S, ')')){
      parse_error(S, "Expected closing parenthesis after 'def ...'");
    }
    if (!P_accept(S, ':')){
      parse_error(S, "Expected colon after 'def ... (...)'");
    }
    if (!P_accept(S, '\n')){
      parse_error(S, "Expected newline after colon ':'");
    }
    nodep block = p_block(S);
    root = new_nodeplrt(new_nodeplrt(name, args, EK_AST_DEF),
			block,
			EK_AST_DEF);
  }
  return root;
}

static nodep p_parameters(parse_state* S){
  /*
    parameter nodes are:
    PARAM L-> VAR
          R-> parameters (or NULL)
   */
  nodep root = NULL;
  nodep arg = NULL;
  nodep n = NULL;
  if ((arg = p_VARIABLE(S)) != NULL){
    root = new_nodeplrt(arg,
		       NULL,
		       EK_AST_PARAM);
    arg = root;
    while (P_accept(S, ',')){
      n = p_VARIABLE(S);
      if (n == NULL){
	parse_error(S, "Expected variable name after comma '(...,'");
      }
      arg->right = new_nodeplrt(n,
				NULL,
				EK_AST_PARAM);
      arg = arg->right;
    }
  }
  return root;
}


static nodep p_whilestmt(parse_state* S){
  /* 
     while nodes are structured as follows
     WHILE L-> COND 
           R-> block
  */
  nodep root = NULL;
  if (P_accept(S, EK_AST_WHILE)){
    nodep expr = p_expression(S);
    if (!expr){
      parse_error(S, "Expected expression after 'while'");
    }
    if (!P_accept(S, ':')){
      parse_error(S, "Expected colon after 'while ...'");
    }
    if (!P_accept(S, '\n')){
      parse_error(S, "Expected newline after colon ':'");
    }
    nodep block = p_block(S);
    root = new_nodeplrt(expr,
			block,
			EK_AST_WHILE);
  }
  return root;
}


static nodep p_ifstmt(parse_state* S){
  /*
    if nodes are structured as follows:
    IF L-> condition
       R-> IFNODE L-> block
                  R-> IF L-> condition
                         R-> IFNODE L-> block
	                            R-> ...
      at one point in the tree, the odd right node will be (nil) or an ELSE
      ELSE L-> (nil)
           R-> block
     */
  nodep root = NULL;
  nodep lastif = NULL;
  if (P_accept(S, EK_AST_IF)){
    int block_indent = S->current_indent;
    nodep condition = p_expression(S);
    if (!P_accept(S, ':')){
      parse_error(S, "Expected colon ':'");
    }
    if (!P_accept(S, '\n')){
      parse_error(S, "Expected newline after colon ':'");
    }
    nodep block = p_block(S);
    nodep ifnode = new_nodeplrt(block, NULL, EK_AST_IFNODE);
    lastif = ifnode;
    //printf("IF block:\n");
    //_astp(block, 0);
    root = new_nodeplrt(condition, ifnode, EK_AST_IF);

    //printf("Got if ___ %.*s\n",10, S->strptr);
    while (1){
      
      int indent = p_INDENT(S);
      if (indent != block_indent){
	if (indent >= 0 && indent < block_indent){
	  // we saw an indentation token, but it didn't belong to this
	  // if..else chain, so we rewind and return
	  P_rewind(S, indent * S->used_indent);
	  break;
	}
	parse_error(S, "Unexpected indentation!");
      }
      if (P_accept(S, EK_AST_ELSE)){
	//printf("Else!\n");
	if (P_accept(S, EK_AST_IF)){
	  //printf("goto\n");
	  goto elif_case;
	}
	if (!P_accept(S, ':')){
	  parse_error(S, "Expected colon ':'");
	}
	if (!P_accept(S, '\n')){
	  parse_error(S, "Expected newline after colon ':'");
	}
	nodep block = p_block(S);
	nodep elsenode = new_nodeplrt(block, NULL, EK_AST_ELSE);
	lastif->right = elsenode;
	break;
      }
      if (P_accept(S, EK_AST_ELIF)){
	nodep condition;
      elif_case:
	condition = p_expression(S);
	if (!P_accept(S, ':')){
	  parse_error(S, "Expected colon ':'");
	}
	if (!P_accept(S, '\n')){
	  parse_error(S, "Expected newline after colon ':'");
	}
	nodep block = p_block(S);
	nodep ifnode = new_nodeplrt(block, NULL, EK_AST_IFNODE);
	lastif->right = new_nodeplrt(condition, ifnode, EK_AST_IF);
	lastif = ifnode;
	continue;
      }
      break;
    }

  }
  return root;
}


static nodep p_retstmt(parse_state* S){
  if (P_accept(S, EK_AST_RETURN)){
    nodep e = p_expression(S);
    //printf("return node\n");
    //ek_parse_print_ast(e);
    if (!e){
      parse_error(S, "Expected expression after return statement");
    }
    return new_nodeplrt(e, NULL, EK_AST_RETURN);
  }
  return NULL;
}

static nodep p_assignment(parse_state* S){
  nodep root;
  if ((root = p_expression(S)) == NULL){
    return NULL;
  }
  if (P_accept(S, '=')){
    nodep expr = p_expression(S);
    nodep ass = new_nodeplrt(root, expr, EK_AST_ASSIGN);
    return ass;
  }
  return root;  
}

static nodep p_expression(parse_state* S){
  return p_addition(S);
}

static nodep p_addition(parse_state* S){
  nodep root;
  if ((root = p_multiplication(S)) == NULL){
    //printf("Error, p_call returned NULL in p_add\n");
    return NULL;
  }
  if (P_accept(S,'+')){
    nodep right = p_addition(S);
    nodep left = root;
    root = new_nodeplrt(left, right, EK_AST_ADD);
  }
  else if (P_accept(S,'-')){
    nodep right = p_addition(S);
    nodep left = root;
    root = new_nodeplrt(left, right, EK_AST_SUB);
  }
  else if (P_accept(S,'<')){
    nodep right = p_addition(S);
    nodep left = root;
    root = new_nodeplrt(left, right, EK_AST_LESS);
  }
  /*
  else if (P_accept(S,'>')){
    nodep right = p_addition(S);
    nodep left = root;
    root = new_nodeplrt(left, right, EK_AST_LESS);
    }*/
  return root;
}

static nodep p_multiplication(parse_state* S){
  nodep root;
  if ((root = p_power(S)) == NULL){
    //printf("Error, p_call returned NULL in p_add\n");
    return NULL;
  }
  if (0){
    if (P_accept(S,'*')){
      nodep right = p_multiplication(S);
      nodep left = root;
      root = new_nodeplrt(left, right, EK_AST_MUL);
    }
    else if (P_accept(S,'/')){
      nodep right = p_multiplication(S);
      nodep left = root;
      root = new_nodeplrt(left, right, EK_AST_DIV);
    }
  }
  while (1){
    if (P_accept(S,'*')){
      nodep left = root;
      nodep right = p_power(S);
      root = new_nodeplrt(left, right, EK_AST_MUL);
    }
    else if (P_accept(S,'/')){
      nodep left = root;
      nodep right = p_power(S);
      root = new_nodeplrt(left, right, EK_AST_DIV);
    }
    else{
      break;
    }
  }
  return root;
}

static nodep p_power(parse_state* S){
  nodep root;
  if ((root = p_value(S)) == NULL){
    return NULL;
  }
  nodep trailer;
  while ((trailer = p_trailer(S)) != NULL){
    trailer->left = root;
    root = trailer;
  }
  return root;
}

static nodep p_trailer(parse_state* S){
  nodep root = NULL;
  if (P_accept(S, '(')){
    // here we are parsing a function call!
    //printf("Got ( %.*s\n",3,S->strptr);
    nodep right = p_expression(S);
    root = new_nodeplrt(NULL, right,EK_AST_CALL);
    //printf("looking for ) %.*s\n",3,S->strptr);
    if (!P_accept(S, ')')){
      printf("Error, missing left ) in p_call\n");
    }
  }
  else if (P_accept(S, '.')){
    nodep str = p_VARIABLE(S);
    if (!str){
      printf("Error, found . with no valid attribute\n");
      return NULL;
    }
    root = new_nodeplrt(NULL, str, EK_AST_DOT);
  }
  return root;
}

static nodep p_value(parse_state* S){
  if (P_accept(S, '(')){
    // here we are parsing an (expression)
    nodep root = p_expression(S);
    if (!P_accept(S, ')')){
      printf("Error, missing left ) in expression\n");
    }
    return root;
  }
  else if (P_accept(S, '"')){
    //printf("Saw strstart\n");
    nodep str = new_nodep();
    char* s = S->strptr;
    int escapes = 0;
    while (*s != '"' || escapes){
      escapes = 0;
      if (*s == '\\'){
	escapes = 1;
      }
      s++;
    }
    str->toklen = s-S->strptr;
    str->tokstr = S->strptr;
    str->type = EK_AST_CSTSTR;
    S->strptr = s+1;
    return str;
  }else if (P_see(S, EK_AST_VARNAME)){
    
    nodep str = new_nodep();
    char* s = S->strptr;
    while (P_seevalid_id(S)){
      S->strptr++;
    }
    str->toklen = S->strptr-s;
    str->tokstr = s;
    str->type = EK_AST_VARNAME;/*
    printf("ID:");
    printnodep(str);
    puts("");*/
    return str;
  }else if (P_see(S, EK_AST_NUMBER)){
    //puts("Taking NUMBER");
    nodep num = new_nodep();
    char* s = S->strptr;
    while (*s <= '9' && *s >= '0'){
      s++;
    }
    num->toklen = s-S->strptr;
    num->tokstr = S->strptr;
    num->type = EK_AST_CSTINT;
    //printf("number %.*s\n",num->toklen, num->tokstr);
    S->strptr = s;
    //printf("left %.*s\n",10, S->strptr);
    return num;
  }
  //printf("No suitable p_value found\n");
  return NULL;
}

 static nodep p_VARIABLE(parse_state* S){

    if (P_see(S, EK_AST_VARNAME)){
      
      nodep str = new_nodep();
      char* s = S->strptr;
      while (P_seevalid_id(S)){
	S->strptr++;
      }
      str->toklen = S->strptr-s;
      str->tokstr = s;
      str->type = EK_AST_VARNAME;
      return str;
    }
    return NULL;
 }

/*
static nodep p_attribute(parse_state* S){
  nodep root;
  if ((root = p_value(S)) == NULL){
    return NULL;
  }
  if (P_accept(S, '.')){
    if (P_see(S, EK_AST_VARNAME)){
      
      nodep str = new_nodep();
      char* s = S->strptr;
      while (P_seevalid_id(S)){
	S->strptr++;
      }
      str->toklen = S->strptr-s;
      str->tokstr = s;
      str->type = EK_AST_VARNAME;
     
      nodep right = str;
      nodep left = root;
      root = new_nodeplrt(left, right, EK_AST_DOT); 
    }else{
      printf("Error, found . with no valid attribute\n");
      return NULL;
    }
  }
  return root;
}
*/
