#include <Ekanz.h>

#include <stdio.h>
#include <stdlib.h>

char* ek_raw_readfile(char* filename){
  FILE* fp = fopen(filename, "r");
  if (fp == NULL){
    fprintf(stderr, "Error, could not open file %s\n",filename);
    return NULL;
  }
  fseek(fp, 0, SEEK_END);
  size_t size = ftell(fp);
  rewind(fp);
  char* data = malloc(size);
  size_t n_read = fread(data, 1, size, fp);
  if (size != n_read){
    fprintf(stderr, "Warning, could not read whole file %s, read %ld bytes (file has %d).\n", filename, n_read,
	    size);
  }
  return data;
}
