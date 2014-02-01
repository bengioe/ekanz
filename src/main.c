#include <stdio.h>
#include <Ekanz.h>

int main(int argc, char** argv){
  printf("Ekanz v0.0.1 (int%d,long%d)\n",sizeof(int),sizeof(long));
  char* file_data = ek_raw_readfile("test.py");
  //printf("read: %s\n", file_data);
  ek_ast_node* ast = ek_parse_text(file_data, "test.py");
  ek_parse_print_ast(ast);
  ek_bytecode* bc = ek_bc_compile_ast(ast);
  
  //ek_native_compile_bc(bc);
  return 0;
}
