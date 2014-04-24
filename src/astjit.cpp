extern "C"{
#include <Ekanz.h>
}
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <stack>
#include <string>
#include <vector>

typedef ek_ast_node* anp;

typedef struct scopeItem{
  uint32_t scoperef;
  uint32_t scopepos;
} scoperef;

class FlagPass{
public:

  std::map<std::string, scopeItem>* globals;
  std::stack<std::map<std::string, scopeItem>*> locals_stack;
  std::stack<uint32_t> locals_size;
  anp root;
  std::vector<int64_t> variable_flags;
  std::vector<std::string> variable_names;
  uint32_t variable_count;
  bool verbose = false;
  

  FlagPass(anp r){
    root = r;
    globals = new std::map<std::string, scopeItem>();
    locals_size.push(0);
    variable_count = 0;
  }

  void visit(anp n, anp parent, bool left = true){
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
      visit(n->left,n, true); visit(n->right,n, false); break;
    case EK_AST_BLOCK_NODE:
      visit(n->left,n); if (n->right) visit(n->right,n); break;

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

  void operator()(bool verbose = false){
    this->verbose = verbose;
    locals_stack.push(globals);

    visit(root, root);
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

  ek_parse_print_ast(root);

  FlagPass first_pass(root);
  first_pass(true);

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
