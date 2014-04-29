extern "C"{
#include <Ekanz.h>
}
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>

typedef ek_ast_node* anp;

typedef struct scopeItem{
  uint32_t scoperef;
  uint32_t scopepos;
} scoperef;


class X86StackMachine{
public:
  std::vector<uint8_t> code;
  /*
    rax    accumulator
    rdx    local frame pointer
    rsp    top of local stack

    rdi, true rdx
    rsi, true rsp
    rcx, r8, temp
   */
  X86StackMachine(){
    // cdecl args are:
    // rdi, rsi, rdx, rcx, r8, r9 + stack

    c(0x48, 0x89, 0xd0); // mov %rdx, %rax
    c(0x48, 0x89, 0xfa); // mov %rdi, %rdx
    c(0x48, 0x89, 0xc7); // mov %rax, %rdi
	
    c(0x48, 0x89, 0xe0); // mov %rsp, %rax
    c(0x48, 0x89, 0xf4); // mov %rsi, %rsp
    c(0x48, 0x89, 0xc6); // mov %rax, %rsi

  }

  void endBlock(){
    c(0x48, 0x89, 0xe0); // mov %rsp, %rax
    c(0x48, 0x89, 0xfa); // mov %rdi, %rdx
    c(0x48, 0x89, 0xf4); // mov %rsi, %rsp
    c(0xc3);// ret
  }

  char* allocBlock(){
    char* block = (char*)mmap(NULL, code.size(), 
			      PROT_EXEC|PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    std::copy(code.begin(), code.begin()+code.size(), block);
    if (0){
      for(int i=18;i<code.size()-10;i++){
	char c = block[i];
	printf("%02x ",(unsigned char)c);
      }
      puts("");
    }
    return block;
  }

  void c(uint8_t a){
    code.push_back(a);
  }
  void c(uint8_t a, uint8_t b){
    c(a); code.push_back(b);
  }
  void c(uint8_t a, uint8_t b, uint8_t x){
    c(a,b); c(x);
  }
  void c(uint8_t a, uint8_t b, uint8_t x, uint8_t y){
    c(a,b); c(x,y);
  }
  void c32(int v){
    c(v & 0xff, v >> 8 & 0xff, v>>16 & 0xff, v>>24 & 0xff);
  }

  void popIntoGlobal(int pos){
    printf("///////////////\n");
  }
  void popIntoLocal(int pos){
    // pop %rax
    c(0x58);
    // mov %rax, pos*8(%rdx)
    c(0x48, 0x89, 0x42, (pos+1) * -8);
  }
  
  void pushGlobal(int pos){
    printf("///////////////\n");
  }
  void pushLocal(int pos){
    // mov pos*8(%rdx), %rax
    c(0x48, 0x8b, 0x42, (pos+1) * -8);
    // push %rax
    c(0x50);
  }

  void pushInt32(int v){
    c(0x68);
    c32(v);
  }

  void add(){
    // pop %rax
    c(0x58);
    // add (%rsp), %rax
    c(0x48, 0x03, 0x04, 0x24);
    // mov %rax, (%rsp)
    c(0x48, 0x89, 0x04, 0x24);
  }

  // see add for explanation
  void sub(){
    c(0x58); c(0x48, 0x2b, 0x04, 0x24); c(0x48, 0x89, 0x04, 0x24);  }
  void mul(){
    c(0x58); c(0x48, 0x0f, 0xaf, 0x04); c(0x24); c(0x48, 0x89, 0x04, 0x24);  }
  void div(){
    /*
      pop %rax       # lower bits of dividend
      mov %rdx, %r8  # keep rdx
      pop %rcx       # divisor
      xor %rdx, %rdx # set higher bits to 0
      idiv %rcx      # result is in %rax
      mov %r8, %rdx  # restore  */
    c(0x59);
    c(0x49, 0x89, 0xd0);
    c(0x58); 
    c(0x48, 0x31, 0xd2);
    c(0x48, 0xf7, 0xf9);
    c(0x4c, 0x89, 0xc2);
    // push %rax
    c(0x50); }

  void mod(){
    /*
      pop %rax       # lower bits of dividend
      mov %rdx, %r8  # keep rdx
      pop %rcx       # divisor
      xor %rdx, %rdx # set higher bits to 0
      idiv %rcx      # result is in %rax
      mov %r8, %rdx  # restore  */
    c(0x58); 
    c(0x49, 0x89, 0xd0);
    c(0x59);
    c(0x48, 0x31, 0xd2);
    c(0x48, 0xf7, 0xf9);
    c(0x4c, 0x89, 0xc2);
    // push %rdx
    c(0x52);  }

  void ret(){
    c(0xc3);
  }

};

class TreePasses{
public:

  std::map<std::string, scopeItem>* globals;
  std::stack<std::map<std::string, scopeItem>*> locals_stack;
  std::stack<uint32_t> locals_size;
  anp root;
  std::vector<int64_t> variable_flags;
  std::vector<std::string> variable_names;
  uint32_t variable_count;
  bool verbose = false;
  

  TreePasses(anp r){
    root = r;
    globals = new std::map<std::string, scopeItem>();
    locals_size.push(0);
    variable_count = 0;
  }

  const long IS_LEAF_BN = 1;

  int fp_visit(anp n, anp parent, bool left = true){
    n->local_scope_size = 0;
    n->isCompiledUnit = 0;
    int flag = 0;
    auto setflag = [&] (uint32_t id, long flag) {
      variable_flags[id] |= 1 << flag;
    };
    switch(n->type){
      // jit supported:
    case EK_AST_ASSIGN:
    case EK_AST_ADD:
    case EK_AST_SUB:
    case EK_AST_MUL:
    case EK_AST_DIV:{
      flag = fp_visit(n->left,n, true) & fp_visit(n->right,n, false);  break;
    }
      // not jit supported:
    case EK_AST_WHILE:
    case EK_AST_LESS:
    case EK_AST_CALL:{
      flag = fp_visit(n->left,n, true) & fp_visit(n->right,n, false); 
      flag &= ~IS_LEAF_BN; break;
    }
    case EK_AST_BLOCK_NODE:{
      int f = fp_visit(n->left,n);
      if (n->right) f &= fp_visit(n->right,n); 
      flag |= f; 
      flag &= ~IS_LEAF_BN;break;}

    case EK_AST_CSTINT:
    case EK_AST_CSTSTR:
      flag |= IS_LEAF_BN;
      break;
    case EK_AST_VARNAME:{
      check_scope(n);
      switch(parent->type){
      case EK_AST_ADD: setflag(n->x.var.scoperef, F_OP_ADD); break;
      case EK_AST_SUB: setflag(n->x.var.scoperef, F_OP_SUB); break;
      case EK_AST_MUL: setflag(n->x.var.scoperef, F_OP_MUL); break;
      case EK_AST_DIV: setflag(n->x.var.scoperef, F_OP_DIV); break;
      case EK_AST_LESS: setflag(n->x.var.scoperef, F_OP_INEQ); break;
      case EK_AST_CALL: // if we're left, we're calling n, else n is the argument of a call
	if (left) setflag(n->x.var.scoperef, F_OP_CALL); 
	else      setflag(n->x.var.scoperef, F_ARG_CALL); break;
      }
      flag |= IS_LEAF_BN;
      break;}
 
    default:
      printf("ast visit unknown %d\n", n->type);
    }
    if (n->type != EK_AST_VARNAME){
      if (flag & IS_LEAF_BN)
	n->x.jit.isLeafUnit = 1;
      else
	n->x.jit.isLeafUnit = 0;
    }
    if (n->type == EK_AST_BLOCK_NODE){
      n->local_scope_size = locals_size.top();
    }
    return flag;
  }

  void check_scope(anp n){
    std::string s(n->tokstr, n->toklen);
    std::map<std::string, scopeItem>& scope = *locals_stack.top();
    if (scope.find(s) == scope.end()){
      scope[s] = {variable_count++, locals_size.top()++};
      variable_flags.push_back(0);
      variable_names.push_back(s);
      if (verbose){
	//printf("var %.*s %d\n",n->toklen,n->tokstr,scope[s].scoperef);
      }
    }
    n->x.var.scoperef = scope[s].scoperef;
    n->x.var.scopepos = scope[s].scopepos;
  }

  void firstPass(bool verbose = false){
    this->verbose = verbose;
    locals_stack.push(globals);

    fp_visit(root, root);
    if (verbose){
      const char* fl[] = {"add","sub","mul","div",
			  "mod","bitw","attr","indx",
			  "called","eq","ineq","len",
			  "range","for","iter","indx",
			  "callarg","if","while","retr",
			  "reslt","local","1l","undsc",
			  "int","float","str","list",
			  "tuple","dict","obj","invalid",
			  "max?"};
    
      auto its = variable_names.begin();
      for (auto it = variable_flags.begin();
	   it != variable_flags.end(); ++it, its++){
	printf("%-20s ",its->c_str());
	for (int j=0;j<64;j++){
	  if (((*it) >> j) & 1){
	    printf("%s ",fl[j]);
	  }
	}
	puts("");
      }
    }
  }
  
  void predictTypes(){
    std::function<void(anp,anp,bool)> visit = [&](anp n, anp parent, bool left = true){
      
      switch(n->type){
      case EK_AST_WHILE:
      case EK_AST_ASSIGN:
      case EK_AST_LESS:
      case EK_AST_ADD:
      case EK_AST_SUB:
      case EK_AST_MUL:
      case EK_AST_DIV:
      case EK_AST_CALL:
      visit(n->left,n, true); visit(n->right,n, false); break;
      case EK_AST_BLOCK_NODE:
      visit(n->left,n, true); if (n->right) visit(n->right,n,true); break;

      case EK_AST_CSTINT:
      case EK_AST_CSTSTR:
	break;
      case EK_AST_VARNAME:{
	// in reality here we would use a model to predict the flags (incl. type)
	switch(parent->type){
	case EK_AST_ADD: 
	case EK_AST_SUB:
	case EK_AST_MUL:
	case EK_AST_DIV:
	case EK_AST_LESS: n->x.var.predicted_type = ek_IntType; break;
	case EK_AST_CALL: // if we're left, we're calling n, else n is the argument of a call
	  if (left) n->x.var.predicted_type = ek_FuncType;
	  else      n->x.var.predicted_type = ek_IntType; break;
	}
	break;}
 
      default:
	printf("ast visit unknown %d\n", n->type);
      }
    };
    
    visit(root,root,true);
  }


  void compileUnit(anp n){
    //printf("Compiling node: \n");
    //printAst(n);
    
    X86StackMachine cc;

    std::function<void(anp)> generateCode = [&] (anp n){
      switch (n->type){
      case EK_AST_ASSIGN:{
	if (n->left->type != EK_AST_VARNAME){
	  printf("Error, don't know how to assign to non-variable!\n");
	}
	generateCode(n->right);
	if (n->x.var.scopepos < 0)
	  cc.popIntoGlobal(-n->left->x.var.scopepos);
	else
	  cc.popIntoLocal(n->left->x.var.scopepos);
	break;
      }
      case EK_AST_CSTINT:{
	int64_t v = atoi(n->tokstr);
	cc.pushInt32(v);
	break;
      }
      case EK_AST_VARNAME:{
	if (n->x.var.scopepos < 0)
	  cc.pushGlobal(-n->x.var.scopepos);
	else
	  cc.pushLocal(n->x.var.scopepos);
	break;
      }
      case EK_AST_ADD:{ generateCode(n->left);generateCode(n->right); cc.add(); break; }
      case EK_AST_SUB:{ generateCode(n->right);generateCode(n->left); cc.sub(); break; }
      case EK_AST_MUL:{ generateCode(n->left);generateCode(n->right); cc.mul(); break; }
      case EK_AST_DIV:{ generateCode(n->left);generateCode(n->right); cc.div(); break; }
      /*case EK_AST_LESS:{ 
	generateCode(n->left);
	generateCode(n->right);
	cc.div(); break; }*/
      default:
      printf("Unhandled type %d\n",n->type);
      }
    };
    generateCode(n);
    cc.endBlock();
    n->x.jit.block = cc.allocBlock();
    n->isCompiledUnit = 1;
  }

  void compileUnits(){
    std::function<void(anp)> findLeaf = [&](anp n){
      if (n->x.jit.isLeafUnit 
	  && n->type != EK_AST_VARNAME
	  && n->type != EK_AST_CSTINT){
	compileUnit(n);
      } else{
	if (n->left)
	  findLeaf(n->left);
	if (n->right)
	  findLeaf(n->right);
      }
    };
    findLeaf(root);
  }


  void run(int64_t* vstack, int64_t* vsp){

    std::function<void(anp)> exec = [&] (anp n){    
      //for (int i=0;i<10;i++){printf("%d ", vstack[-i-1]);}puts("");
      if (n->isCompiledUnit){
	//printf("%p %d\n",vsp,*(vsp-1),vstack);
	int64_t* (*f)(int64_t*,int64_t*) =(int64_t* (*)(int64_t*,int64_t*)) n->x.jit.block;
	vsp = f(vstack, vsp);
	//for (int i=0;i<10;i++){printf("%d ", vstack[-i-1]);}puts("");
	//printf("%p %d\n",vsp,*(vsp-1));
	return;
      }
      switch(n->type){
      case EK_AST_BLOCK_NODE:{
	exec(n->left);
	if (n->right)
	  exec(n->right);
	break;
      }
      case EK_AST_CALL:{
	exec(n->right);
	printf("call %.*s with %d\n",n->left->toklen, n->left->tokstr,
	       *vsp++);
	break;
      }
      
      case EK_AST_WHILE:{
	while (1){
	  exec(n->left);
	  int64_t cond = *vsp++;
	  if (cond != 0){
	    exec(n->right);
	  }else{
	    break;
	  }
	}break;
      }

      case EK_AST_LESS:{
	exec(n->left);
	exec(n->right);
	int64_t r = *vsp++;
	int64_t l = *vsp++;
	if (l < r)
	  *--vsp = 1;
	else
	  *--vsp = 0;
	break;
      }

      case EK_AST_VARNAME:{
	// todo, can't be zero to differenciate global and local... add 1 everywhere!
	if (n->x.var.scopepos >= 0)
	  *--vsp = vstack[-n->x.var.scopepos-1];
	break;}
      case EK_AST_CSTINT:{
	int v = atoi(n->tokstr);
	*--vsp = v;
	break;}
        
      default:
        printf("Unknown node at slow eval %d\n", n->type);
      }
    };
    //printAst(root, true);
    vsp -= root->local_scope_size;
    exec(root);
  }


  void printAst(anp node=NULL, bool hideLeaves=false){
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
      "invalid_",
    };

    std::function<void(anp,int)> _astp = [&] (anp n, int level){
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
      if (n->type == EK_AST_VARNAME){
	printf(" (%d %d ", n->x.var.scoperef, n->x.var.scopepos);
	const char* fl[] = {"add","sub","mul","div",
			    "mod","bitw","attr","indx",
			    "called","eq","ineq","len",
			    "range","for","iter","indx",
			    "callarg","if","while","retr",
			    "reslt","local","1l","undsc",
			    "int","float","str","list",
			    "tuple","dict","obj","invalid",
			    "max?"};
	int64_t f = variable_flags[n->x.var.scoperef];
	for (int j=0;j<64;j++){
	  if (f >> j & 1){
	    printf("%s ",fl[j]);
	  }
	}
	printf("\b)");
      }
      if (hideLeaves && n->x.jit.isLeafUnit){
	printf(" -- leaf\n");
	return;
      }
    	
      printf("\n");
      if (n->type != EK_AST_BLOCK_NODE) level++;
      if (n->left){
	_astp(n->left, level);
      }
      if (n->right){
	_astp(n->right, level);
      }
    };
    if (node == NULL)
      node = root;
    _astp(node,0);
  }

};

void __main(anp root){
  // value
  int64_t* vstack_start = (int64_t*)malloc(1024*1024);
  int64_t* vstack = vstack_start + 1024*1024/8;
  int64_t* vsp = vstack;
  int64_t* vframep = vsp;
  printf("Stack: %p %p\n", vstack_start, vsp);
  // type
  ek_type** tstack = (ek_type**)malloc(1024*1024);
  ek_type** tsp = tstack;
  ek_type** tframep = tsp;
  // backtrace + framepointers
  int64_t* btstack = (int64_t*)malloc(1024*1024);
  int64_t* btsp = btstack;

  // ast node stack
  anp* anstack = (anp*)malloc(1024*1024);
  anp* ansp = anstack;

  //ek_parse_print_ast(root);

  TreePasses tp(root);
  tp.firstPass(true);
  tp.predictTypes();
  //tp.printAst();
  tp.compileUnits();
  tp.run(vstack, vsp);

  *btsp++ = -1;
  *btsp++ = (int64_t)tsp;
  *btsp++ = (int64_t)vsp;
  *btsp = 0;
  
}

extern "C"{

void ek_ast_jitandrun(anp root){
  printf("Entering JIT\n");
  
  __main(root);

  printf("Exiting JIT\n");
}

}
