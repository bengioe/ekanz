
// linked stack of dicts with sentinel
typedef struct scopechain_t{
  strdict_node* scope;
  struct scopechain_t* next;
} scopechain_t;

static scopechain_t* scopechain_new(){
  scopechain_t* s = malloc(sizeof(scopechain_t));
  s->scope = NULL;
  s->next = NULL;
  return s;
}

static int64_t scopechain_contains(astnp varname){
  if (varname->type != EK_AST_VARNAME){
    printf("Error, scopechain_contains received a non-varname node\n");
  }
  return 1;
}

static scopechain_t* scopechain_pushscope(scopechain_t* scopes){
  scopechain_t* s = malloc(sizeof(scopechain_t));
  s->scope = strdict_new();
  s->next = scopes;
  return s;
}

static void scopechain_popscope(scopechain_t* scopes){
  
}

static int64_t scopechain_get(scopechain_t* scopes, astnp varname, int64_t* to){
  if (varname->type != EK_AST_VARNAME){
    printf("Error, scopechain_get received a non-varname node\n");
  }
  while (scopes->scope != NULL && 
	 !strdict_getn(scopes->scope, varname->tokstr, varname->toklen, to)){
    scopes = scopes->next;
  }
  if (scopes->scope == NULL){
    return 0;
  }
  return 1;
}

static int64_t scopechain_set(scopechain_t* scopes, astnp varname, int64_t val){
  if (varname->type != EK_AST_VARNAME){
    printf("Error, scopechain_get received a non-varname node\n");
  }
  strdict_setn(scopes->scope, varname->tokstr, varname->toklen, val);
  return 1;
}

static int64_t scopechain_setn(scopechain_t* scopes, char* k, int64_t len, int64_t val){
  strdict_setn(scopes->scope, k, len, val);
  return 1;
}
