#include <stdlib.h>
#include <Ekanz.h>
#include <stdio.h>

#define max(a,b) (a > b ? a : b)

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

  *btsp++ = -1;
  *btsp++ = (int64_t)tsp;
  *btsp++ = (int64_t)vsp;
  *btsp = 0;
  char* pc = bc->startptr;
  char* error_str;

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
      break;}
    
    case PUSH_GLOBAL:{
      int32_t v = *(int32_t*)pc; pc+=4;
      //printf("push_from %d (%d)\n", v, vstack[v]);
      *vsp++ = vstack[v];
      *tsp++ = tstack[v];
      break;}

    case PUSH_LOCAL:{
      int32_t v = *(int32_t*)pc; pc+=4;
      //printf("push_from %d (%d)\n", v, vstack[v]);
      *vsp++ = vframep[v];
      *tsp++ = tframep[v];
      break;}

    case POP_INTO_GLOBAL:{
      int32_t v = *(int32_t*)pc; pc+=4;
      //printf("pop %d into %d\n",*(vsp-1),v);
      vstack[v] = *--vsp;
      tstack[v] = *--tsp;
      break;}

    case POP_INTO_LOCAL:{
      int32_t v = *(int32_t*)pc; pc+=4;
      //printf("pop %d into %d\n",*(vsp-1),v);
      vframep[v] = *--vsp;
      tframep[v] = *--tsp;
      break;}

    case POPN:{
      int32_t v = *(int32_t*)pc; pc+=4;
      vsp -= v;
      tsp -= v;
      break;}

    case ADD:{
      int64_t a = *--vsp;
      int64_t b = *--vsp;
      ek_type* at = *--tsp;
      ek_type* bt = *--tsp;
      if (at == bt && bt == ek_IntType){
	*vsp++ = a + b;
	*tsp++ = ek_IntType;
      }else{
	error_str = "trying to add two non-numbers";
	goto fatalerror;
      }
      break;}

    case CALL:{
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
	//printf("C CALL %p\n", a);
	void (*f)(int64_t,ek_type*,int64_t*,ek_type**) = 
	 (void(*)(int64_t,ek_type*,int64_t*,ek_type**))
	  ek_obj_getextra((ekop)a, 0);
	int64_t arg = *--vsp;
	ek_type* argtype = *--tsp;
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
      vsp -= frame_size;
      tsp -= frame_size;
      *vsp++ = rvalue;
      *tsp++ = rvaluet;
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
    printf("end (%d on stack)\n", vstack[0]);
  } else if (1){
 fatalerror:
    printf("Fatal error %s\n", error_str);
  }
}
