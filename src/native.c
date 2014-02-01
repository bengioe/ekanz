#include <Ekanz.h>

#include <sys/mman.h>

int64_t ek_native_compile_bc(ek_bytecode* bc){
  printf("\ncompiling to x64\n\n");
  char* code = mmap(NULL, 1024, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
  char* codestart = code;
  char* p = bc->startptr;
  int (*f)(void) = codestart;
  if (0){
    *code++ = 0x48;
    *code++ = 0xc7;
    *code++ = 0xc0;
    *(int32_t*)code = 42;
    code += 4;
    *code++ = 0xc3;
  }
  // rax is temp & return value, rdx is local stack start
  while(*p != 0){
    printf("%d\n",*p);
    switch(*p++){
    case PUSH:{
      int32_t v = *(int32_t*)p;
      p+=4;
      *code++ = 0x68;
      printf("push %d\n", v);
      *(int32_t*)code = v; code += 4;
      break;
    }
    case PUSH_FROM:{
      // mov v*8(%rdx), %rax
      int32_t v = *(int32_t*)p;
      p+=4;
      *code++ = 0x48; *code++ = 0x8b; *code++ = 0x42;
      *code++ = (v+1) << 3;
      // push %rax
      *code++ = 0x50; break;}
    case POP_INTO:{
      // pop %rax
      *code++ = 0x58;
      // mov %rax, v*8(%rdx)
      int32_t v = *(int32_t*)p;
      p+=4;
      *code++ = 0x48; *code++ = 0x89;*code++ = 0x42;
      *code++ = (v+1)<<3;
      printf("pop into %d\n", v);
      break;}
    case POPN:{
      // sub $v*8, %rsp
      int32_t v = *(int32_t*)p;
      p+=4;
      *code++ = 0x48; *code++ = 0x83;*code++ = 0xc4;//ec;
      *code++ = v<<3;
      printf("popn %d\n", v);
      break;}
    case ADD:{
      // pop %rax
      *code++ = 0x58;
      // add (%rsp), %rax
      //*(int32_t*)code = 0x48030424; code+=4;
      *(int32_t*)code = 0x24040348; code+=4;
      // mov %rax, (%rsp)
      //*(int32_t*)code = 0x48890424; code+=4;
      *(int32_t*)code = 0x24048948; code+=4;
      break;}
    case CALL:{
      // pop %rax  argument
      // pop %rcx  function pointer
      // call *%rcx
      //*(int32_t*)code = 0x5859ffd1; code+=4;    
      *(int32_t*)code = 0xd1ff5859; code+=4;      
      break; }
    case NEW_FRAME:
      *code++=0x48; *code++=0x31;*code++=0xc0;
      // mov %rsp, %rdx
      *code++ = 0x48; *code++ = 0x89;*code++ = 0xe2; break;
    case RETURN:
      *code++ = 0xc3;
      break;
    
    default:
      printf("Unknown opcode %d\n",*--p);
      goto breakout;
    }
  }
  int r;
 breakout:
  r = f();
  printf("return %d\n", r);

  return (int64_t)codestart;
}
