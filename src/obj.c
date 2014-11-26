#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Ekanz.h>



ek_type* ek_NoneType = 0;
ek_type* ek_IntType = 0;
ek_type* ek_FuncType = 0;
ek_type* ek_CFuncType = 0;
ek_type* ek_StrType = 0;
ekop ek_None = 0;




unsigned long xhash(char* s, long l){
  unsigned long h = 14695981039346656037;
  while (l-->0){h = (h ^ *s++) * 1099511628211;}
  return h;
}
unsigned long yhash(char* s, long l){
  unsigned long h = 961748927;
  while (l-->0){h = ((h<<5) + *s++) * 160481183;}
  return h;
}

void ek_init_type(){
  if (ek_NoneType == NULL){
    ek_NoneType = ek_type_new();
    ek_NoneType = ek_type_derive(ek_NoneType, "__str__", 7);
    ek_NoneType = ek_type_derive(ek_NoneType, "__eq__", 6);
    ek_None = ek_type_instanciate(ek_NoneType);
    ek_IntType = ek_type_new();
    ek_FuncType = ek_type_new();
    ek_CFuncType = ek_type_new();
    ek_CFuncType->extra_space = sizeof(void*);
    ek_StrType = ek_type_new();
    ek_StrType = ek_type_derive(ek_StrType, "__str__", 7);
    ek_StrType->extra_space = sizeof(char*) + sizeof(unsigned long);
    // todo: free types here that are derived
  }
}

ek_type* ek_type_new(){
  ek_type* t = malloc(sizeof(ek_type));
  t->map_size = 0;
  t->map = 0;
  t->nentries = 0;
  t->entry = NULL;
  return t;
}


ek_type* ek_type_derive(ek_type* t, char* s, long l){
  ek_type_entry* e = malloc(sizeof(ek_type_entry));
  e->str = s;
  e->strlen = l;
  e->xhash = xhash(s,l);
  e->yhash = yhash(s,l);
  e->next = t->entry;
  
  ek_type* newt = ek_type_new();
  newt->entry = e;
  newt->nentries = t->nentries + 1; 

  // here we want to create a perfect hash map for the type
  int map_size = t->map_size + 1;
  int indexes[newt->nentries];
  int hasCollision;
  do{
    hasCollision = 0;
    // for each entry we're going to insert their xhash % map_size and see if there is collision
    // increase the map size
    char tmap[map_size], j;
    for (j=0;j<map_size;j++){tmap[j]=0;}
    ek_type_entry* i = newt->entry;
    while (i != NULL){
      unsigned long h = (i->xhash) % map_size;
      if (tmap[h] != 0){
	hasCollision = 1;
	map_size++;
	break;
      }
      i = i->next;
      tmap[h] = 1;
    }
  } while(hasCollision);
  
  newt->map_size = map_size;
  newt->map = malloc(map_size);
  ek_type_entry* i = newt->entry;
  int cnt = 0;
  while (i != NULL){
    unsigned long h = (i->xhash) % map_size;
    newt->map[h] = cnt++;
    i = i->next;
  }
  //printf("new type (%d entries, map size is %d)\n", newt->nentries, map_size);
  return newt;
}


ek_type* ek_type_deriveN(ek_type* t, char** s, long n){
  int k;
  ek_type_entry* last = t->entry;
  for (k=0;k<n;k++){
    ek_type_entry* e = malloc(sizeof(ek_type_entry));
    char* str = s[k];
    long len = strlen(s[k]);
    e->str = str;
    e->strlen = len;
    e->xhash = xhash(str,len);
    e->yhash = yhash(str,len);
    e->next = last;
    last = e->next;
  }
  ek_type* newt = ek_type_new();
  newt->entry = last;
  newt->nentries = t->nentries + n; 
  // here we want to create a perfect hash map for the type
  int map_size = t->map_size + n;
  int indexes[newt->nentries];
  int hasCollision;
  do{
    hasCollision = 0;
    // for each entry we're going to insert their xhash % map_size and see if there is collision
    // increase the map size
    char tmap[map_size], j;
    for (j=0;j<map_size;j++){tmap[j]=0;}
    ek_type_entry* i = newt->entry;
    while (i != NULL){
      unsigned long h = (i->xhash) % map_size;
      if (tmap[h] != 0){
	hasCollision = 1;
	map_size++;
	break;
      }
      i = i->next;
      tmap[h] = 1;
    }
  } while(hasCollision);
  
  newt->map_size = map_size;
  newt->map = malloc(map_size);
  ek_type_entry* i = newt->entry;
  int cnt = 0;
  while (i != NULL){
    unsigned long h = (i->xhash) % map_size;
    newt->map[h] = cnt++;
    i = i->next;
  }
  printf("new type (%d entries, map size is %d)\n", newt->nentries, map_size);
  return newt;
}


ekop ek_type_instanciate(ek_type* t){  
  ekop o = malloc(sizeof(ek_type*) + sizeof(ekop) * t->nentries + t->extra_space);
  o->type = t;
  int i;
  ekop* oi = (ekop*)o->data;
  for (i=0;i<t->nentries;i++){
    *oi++ = ek_None;
  }
  return o;
}


void ek_obj_setextra(ekop o, uint64_t pos, ekop v){
  *(((ekop*)o->data) + o->type->nentries + pos) = v;
}

ekop ek_obj_getextra(ekop o, uint64_t pos){
  return *(((ekop*)o->data) + o->type->nentries + pos);
}

void ek_obj_setattr_str(ekop o, char* s, ekop v){
  unsigned long hash = xhash(s, strlen(s)) % o->type->map_size;
  *((ekop*)o->data + o->type->map[hash]) = v;
}

ekop ek_obj_getattr_strn(ekop o, char* s, int l){
  unsigned long hash = xhash(s, l) % o->type->map_size;
  return *((ekop*)o->data + o->type->map[hash]);
}
