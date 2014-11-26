#include <stdio.h>
#include <time.h>
#include <sys/time.h>

double getTimeNow(){
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC,  &t);;
    //time in seconds
    return t.tv_sec + (double) t.tv_nsec * 1e-9;
}

#include <Ekanz.h>


int main(int argc, char** argv){
  //printf("Ekanz v0.0.1 (int%d,long%d)\n",sizeof(int),sizeof(long));

  ek_init_type();

  if (0){
    ek_type* strType = ek_type_derive(ek_NoneType, "__getitem__", 11);
    strType = ek_type_derive(strType, "__len__", 7);
    return 0;
  }

  char* file_data = ek_raw_readfile(argv[1]);
  //printf("read: %s\n", file_data);
  ek_ast_node* ast = ek_parse_text(file_data, "test.py");
  ek_parse_print_ast(ast);
  printf("\n");

  double t0 = getTimeNow();
  ek_bytecode* bc = ek_bc_compile_ast(ast);
  ek_bc_print(bc);
  //puts("");
  //ek_native_compile_bc(bc)

  ek_vm_run(bc);
  double t1 = getTimeNow();
  /*

  extern void ek_ast_jitandrun(ek_ast_node*);
  ek_ast_jitandrun(ast);
  double t2 = getTimeNow();
  printf("BC vm took   %f seconds\nAST JIT took %f seconds\n", t1-t0, t2-t1);
  */
  printf("%f seconds\n",t1-t0);
  return 0;
}
