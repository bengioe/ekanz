

/*
  What is a type?
  
  A type is a strict map from strings to addresses, so types only
  identify WHERE each ATTRIBUTE is stored within a given object of
  that type.

  As so, each object keeps its own references to, say, class methods.

  To be closer to the python spec, we should add read-only flags to
  each attribute.
 */
typedef struct ek_type_entry{
  char* str;
  unsigned int strlen;
  unsigned int index;
  unsigned long xhash;
  unsigned long yhash;
  struct ek_type_entry* next;
} ek_type_entry;

typedef struct ek_type{
  char* map;
  unsigned long map_size;
  unsigned long nentries;
  unsigned int extra_space;
  ek_type_entry* entry; // linked list of entries
} ek_type;



typedef struct ek_obj{
  ek_type* type;
  char data[];
} ek_obj;

typedef ek_obj* ekop;

ek_type* ek_type_new();
ek_type* ek_type_derive(ek_type*,char*,long);
ek_type* ek_type_deriveN(ek_type*,char**,long);
ekop     ek_type_instanciate(ek_type*);

ekop ek_obj_new();
void ek_obj_setattr_str(ekop,char*,ekop);
extern ek_type* ek_NoneType;
extern ek_type* ek_IntType;
extern ek_type* ek_FuncType;
extern ek_type* ek_CFuncType;
extern ek_type* ek_StrType;

extern ekop ek_None;
