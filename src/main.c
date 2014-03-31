#include <stdio.h>
#include <Ekanz.h>


int main(int argc, char** argv){
  //printf("Ekanz v0.0.1 (int%d,long%d)\n",sizeof(int),sizeof(long));

  ek_init_type();

  if (0){
    ek_type* strType = ek_type_derive(ek_NoneType, "__getitem__", 11);
    strType = ek_type_derive(strType, "__len__", 7);
    return 0;
  }

  char* file_data = ek_raw_readfile("test.py");
  //printf("read: %s\n", file_data);
  ek_ast_node* ast = ek_parse_text(file_data, "test.py");
  ek_parse_print_ast(ast);
  printf("\n");
  ek_bytecode* bc = ek_bc_compile_ast(ast);
  ek_bc_print(bc);
  puts("");
  //ek_native_compile_bc(bc)

  ek_vm_run(bc);
  return 0;
}
