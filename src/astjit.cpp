extern "C"{
#include <Ekanz.h>
}
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <functional>

typedef ek_ast_node* anp;

typedef struct scopeItem{
  uint32_t scoperef;
  uint32_t scopepos;
} scoperef;

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

  void fp_visit(anp n, anp parent, bool left = true){
    auto setflag = [&] (uint32_t id, long flag) {
      variable_flags[id] |= 1 << flag;
    };
    switch(n->type){
    case EK_AST_WHILE:
    case EK_AST_ASSIGN:
    case EK_AST_LESS:
    case EK_AST_ADD:
    case EK_AST_SUB:
    case EK_AST_MUL:
    case EK_AST_DIV:
    case EK_AST_CALL:
      fp_visit(n->left,n, true); fp_visit(n->right,n, false); break;
    case EK_AST_BLOCK_NODE:
      fp_visit(n->left,n); if (n->right) fp_visit(n->right,n); break;

    case EK_AST_CSTINT:
    case EK_AST_CSTSTR:
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
      break;}
 
    default:
      printf("ast visit unknown %d\n", n->type);
    }
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



  void printAst(){
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
    	
      printf("\n");
      if (n->type != EK_AST_BLOCK_NODE) level++;
      if (n->left){
	_astp(n->left, level);
      }
      if (n->right){
	_astp(n->right, level);
      }
    };
    _astp(root,0);
  }

};

void __main(anp root){
  // value
  int64_t* vstack = (int64_t*)malloc(1024*1024);
  int64_t* vsp = vstack;
  int64_t* vframep = vsp;
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
  tp.printAst();

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
