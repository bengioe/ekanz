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
  while(*p != 0){
    switch(*p++){
    case PUSH:{
      int64_t v = *(int64_t*)p;
      // todo, verify that v < MAX_32_BIT_VALUE
      p+=8;
      *code++ = 0x68;
      printf("push %d\n", (int32_t)v);
      *(int32_t*)code = (int32_t)v;
      break;
    }
    case RETURN:
      *code++ = 0xc3;
      break;
    
    default:
      printf("Unknown opcode %d\n",*--p);
      goto breakout;
    }
  }
 breakout:
  printf("%p\n",codestart);
  int r = f();
  printf("return %d\n", r);

  return (int64_t)codestart;
}
