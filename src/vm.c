#include <stdlib.h>
#include <Ekanz.h>
#include <stdio.h>

#define max(a,b) (a > b ? a : b)

#define COLLECT_FLAG 0
#if COLLECT_FLAG
#include "opt.c"
#endif

void ek_vm_run(ek_bytecode* bc){
  printf("\nEntering VM\n");
  // value
  int64_t* vstack = malloc(1024*1024);
  int64_t* vsp = vstack;
  int64_t* vframep = vsp;
  // type
  ek_type** tstack = malloc(1024*1024);
  ek_type** tsp = tstack;
  ek_type** tframep = tsp;
  // backtrace + framepointers
  int64_t* btstack = malloc(1024*1024);
  int64_t* btsp = btstack;

#if COLLECT_FLAG
  // stack of variable flags indexes
  int32_t* vfstack = malloc(1024*1024);
  int32_t* vfsp = vfstack;
  int32_t* vfframep = vfsp;
  // the actual variable flags array
  vfp vfa = malloc(1024*1024*20);

  long nvarflags = 1;
#define setvf(x,y) vfa[x].flags |= 1 << y
#define clearvf(x) vfa[x].flags = 0
  clearvf(0);
  setvf(*vfsp++, F_FLAG_INVALID);
#endif

  *btsp++ = -1;
  *btsp++ = (int64_t)tsp;
  *btsp++ = (int64_t)vsp;
#if COLLECT_FLAG
  *btsp++ = (int64_t)vfsp;
#endif
  *btsp = 0;
  char* pc = bc->startptr;
  char* error_str;
  int asdasd = 1;
  while(pc != -1 && pc != 0 && *pc != 0 && *btsp != -1){
    
    if (0){
      printf("pc=%p op=%d \n",pc, *pc);
      int64_t* _p = max(vstack,vsp-20);
      printf("{ ");
      while (_p < vsp){
	if (_p == vframep){
	  printf("^ ");
	}
	printf("%d ", *_p++);
      }
      puts("}");
      _p = max(tstack, tsp-20);
      printf("{ ");
      while (_p < tsp){
	if (_p == tframep){
	  printf("^ ");
	}
	printf("%p ", *_p++);
      }
      puts("}");
    }

    switch(*pc++){
    case PUSH:{
      int64_t v = *(int64_t*)pc; pc+=8;
      ek_type* t = *(ek_type**)pc; pc+=8;
      *vsp++ = v;
      *tsp++ = t;
#if COLLECT_FLAG
      if (v != 0){ // if we're not pushing an undefined, we're
		   // probably just pushing a constant, which we dont
		   // really care about
	*vfsp++ = 0; // point to dummy var_flags
      }else{
	*vfsp++ = nvarflags++;
	clearvf(nvarflags-1);
      }
#endif
      break;}
    
    case PUSH_GLOBAL:{
      int32_t v = *(int32_t*)pc; pc+=4;
      //printf("push_from %d (%d)\n", v, vstack[v]);
      *vsp++ = vstack[v];
      *tsp++ = tstack[v];
#if COLLECT_FLAG
      *vfsp++ = vfstack[v];
#endif
      break;}

    case PUSH_LOCAL:{
      int32_t v = *(int32_t*)pc; pc+=4;
      //printf("push_from %d (%d)\n", v, vframep[v]);
      *vsp++ = vframep[v];
      *tsp++ = tframep[v];
#if COLLECT_FLAG
      *vfsp++ = vfframep[v];
#endif
      break;}

    case POP_INTO_GLOBAL:{
      int32_t v = *(int32_t*)pc; pc+=4;
      //printf("pop %d into %d\n",*(vsp-1),v);
      vstack[v] = *--vsp;
      tstack[v] = *--tsp;
#if COLLECT_FLAG
      vfstack[v] = *--vfsp;
#endif
      break;}

    case POP_INTO_LOCAL:{
      int32_t v = *(int32_t*)pc; pc+=4;
      //printf("pop %d into %d\n",*(vsp-1),v);
      vframep[v] = *--vsp;
      tframep[v] = *--tsp;
#if COLLECT_FLAG
      vfframep[v] = *--vfsp;
#endif
      break;}

    case POPN:{
      int32_t v = *(int32_t*)pc; pc+=4;
      vsp -= v;
      tsp -= v;
#if COLLECT_FLAG
      vfsp -= v;
#endif
      break;}

    case ADD:{
      int64_t a = *--vsp;
      int64_t b = *--vsp;
      ek_type* at = *--tsp;
      ek_type* bt = *--tsp;
#if COLLECT_FLAG
      setvf(*--vfsp, F_OP_ADD);
      setvf(  *vfsp, F_TYPE_INT);
      setvf(*--vfsp, F_OP_ADD);
      setvf(  *vfsp, F_TYPE_INT);
      setvf(*vfsp++, F_TYPE_INT);
#endif
      if (at == bt && bt == ek_IntType){
	*vsp++ = a + b;
	*tsp++ = ek_IntType;
      }else{
	error_str = "trying to add two non-numbers";
	goto fatalerror;
      }
      break;}

    case SUB:{
      int64_t a = *--vsp;
      int64_t b = *--vsp;
      ek_type* at = *--tsp;
      ek_type* bt = *--tsp;
#if COLLECT_FLAG
      setvf(*--vfsp, F_OP_SUB);
      setvf(  *vfsp, F_TYPE_INT);
      setvf(*--vfsp, F_OP_SUB);
      setvf(  *vfsp, F_TYPE_INT);
      setvf(*vfsp++, F_TYPE_INT);
#endif
      if (at == bt && bt == ek_IntType){
	*vsp++ = b - a;
	*tsp++ = ek_IntType;
      }else{
	error_str = "trying to subtract two non-numbers";
	goto fatalerror;
      }
      break;}

    case MUL:{
      int64_t a = *--vsp;
      int64_t b = *--vsp;
      ek_type* at = *--tsp;
      ek_type* bt = *--tsp;
#if COLLECT_FLAG
      setvf(*--vfsp, F_OP_MUL);
      setvf(  *vfsp, F_TYPE_INT);
      setvf(*--vfsp, F_OP_MUL);
      setvf(  *vfsp, F_TYPE_INT);
      setvf(*vfsp++, F_TYPE_INT);
#endif
      if (at == bt && bt == ek_IntType){
	*vsp++ = a * b;
	*tsp++ = ek_IntType;
      }else{
	error_str = "trying to multiply two non-numbers";
	goto fatalerror;
      }
      break;}

    case DIV:{
      int64_t a = *--vsp;
      int64_t b = *--vsp;
      ek_type* at = *--tsp;
      ek_type* bt = *--tsp;
#if COLLECT_FLAG
      setvf(*--vfsp, F_OP_DIV);
      setvf(  *vfsp, F_TYPE_INT);
      setvf(*--vfsp, F_OP_DIV);
      setvf(  *vfsp, F_TYPE_INT);
      setvf(*vfsp++, F_TYPE_INT);
#endif
      if (at == bt && bt == ek_IntType){
	*vsp++ = b / a;
	*tsp++ = ek_IntType;
      }else{
	error_str = "trying to multiply two non-numbers";
	goto fatalerror;
      }
      break;}

    case LESSTHAN:{
      int64_t a = *--vsp;
      int64_t b = *--vsp;
      ek_type* at = *--tsp;
      ek_type* bt = *--tsp;
#if COLLECT_FLAG
      setvf(*--vfsp, F_OP_INEQ);
      setvf(  *vfsp, F_TYPE_INT);
      setvf(*--vfsp, F_OP_INEQ);
      setvf(  *vfsp, F_TYPE_INT);
      setvf(*vfsp++, F_TYPE_INT);
#endif
      if (at == bt && bt == ek_IntType){
	*vsp++ = b < a ? 1 : 0;
	*tsp++ = ek_IntType;
      }else{
	error_str = "trying to compare two non-numbers";
	goto fatalerror;
      }
      break;}

    case CALL:{
      // a is the function pointer
      int64_t a = *--vsp;
      ek_type* at = *--tsp;
      if (at == ek_FuncType){
	*btsp++ = (int64_t)pc;
	*btsp++ = (int64_t)tframep;
	*btsp++ = (int64_t)vframep;
	vframep = vsp-1;
	tframep = tsp-1;
	//printf("call %p %d\n", a, vframep-vstack);
	pc = (char*)a;
      } else if (at == ek_CFuncType){
	int64_t rvalue = 0;
	ek_type* rtype = ek_NoneType;
	void (*f)(int64_t,ek_type*,int64_t*,ek_type**) = 
	 (void(*)(int64_t,ek_type*,int64_t*,ek_type**))
	  ek_obj_getextra((ekop)a, 0);
	int64_t arg = *--vsp;
	ek_type* argtype = *--tsp;
	printf("C call %p\n",arg);
	f(arg,argtype,&rvalue,&rtype);
	*vsp++ = rvalue;
	*tsp++ = rtype;
      } else{
	error_str = "trying to call a non-function";
	goto fatalerror;
      }
      break; }

    case NEW_FRAME:
      break;

    case BRANCH:{
      int32_t bl = *(int32_t*)pc; pc+=4;
      int32_t ba = *(int32_t*)pc; pc+=4;
      //printf("branching to %d %p\n",bl,ba);
      pc = (char*)ba;
      break;
    }

    case CONDBRANCH:{
      int64_t a = *--vsp;
      ek_type* at = *--tsp;
#if COLLECT_FLAG
      setvf(*--vfsp, F_ARG_IF);
#endif
      int32_t btl = *(int32_t*)pc; pc+=4;
      int32_t bta = *(int32_t*)pc; pc+=4;
      int32_t bfl = *(int32_t*)pc; pc+=4;
      int32_t bfa = *(int32_t*)pc; pc+=4;
      //printf("branching on value %d to %d:%d %p:%p\n",a,btl,bfl,bta,bfa);
      if (a == 0)
	pc = (char*)bfa;
      else
	pc = (char*)bta;
      break;
    }

    case RETURN:{
      int64_t rvalue = *--vsp;
      int64_t rvaluet= *--tsp;
      int32_t frame_size = *(int32_t*)pc; pc+=4;
      vsp -= frame_size - 1;
      tsp -= frame_size - 1;
      *vsp++ = rvalue;
      *tsp++ = rvaluet;
#if COLLECT_FLAG
      setvf(*vfsp++, F_FL_RESULT);
      vfframep = (int32_t*)(*--btsp);
#endif
      vframep = (int64_t*)*--btsp;
      tframep = (ek_type*)*--btsp;
      pc = (char*)*--btsp;
      //printf("returning to %p with value %d\n",pc, rvalue);
      break;
    }

    case GETATTR:{
      char* s = *(char**)pc; pc += sizeof(char*);
      int32_t l = *(int32_t*)pc; pc+=4;
      ekop o = *--vsp;
      ek_type* t = *--tsp;
      ekop ro = ek_obj_getattr_strn(o, s, l);
      *vsp++ = ro;
      *tsp++ = ro->type;
#if COLLECT_FLAG
      setvf(*--vfsp, F_OP_ATTR);
      // here, we're getting an attribute, do we really want to track
      // and optimize it?  or just the variable in which it will be
      // stored (if any), which should already be tracked.
      *vfsp++ = 0;
#endif
      break;
    }
    case LABEL:
      pc += 4;
      break;

    default:
      printf("Unknown opcode %d\n",*--pc);
      goto breakout;
    }
  }

  if (1){
  breakout:
    printf("Exiting VM (%d on stack)\n", vstack[0]);
#if COLLECT_FLAG
    printf("%d varflags (%d max)\n", nvarflags, F_MAX-1);
    int i=0,j;
    char* fl[] = {"add","sub","mul","div",
		  "mod","bitw","attr","indx",
		  "call","eq","ineq","len",
		  "range","for","iter","indx",
		  "call","if","while","retr",
		  "reslt","local","1l","undsc",
		  "int","float","str","list",
		  "tuple","dict","obj","invalid",
		  "max?"};
    for (i=1;i<nvarflags;i++){
      for (j=0;j<64;j++){
	if ((vfa[i].flags >> j) & 1){
	  printf("%s ",fl[j]);
	}
      }
      puts("");
    }
#endif
  } else if (1){
 fatalerror:
    printf("Fatal error %s\n", error_str);
  }
}
