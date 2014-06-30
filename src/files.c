#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "files.h"

int nof_digits(int n)
{
  int c=0;
  do {
    c += 1;
    n /= 10;
  } while(n);
  return c;
}

int nof_lines(char* filename) {
  int nof_lines = 0;
  int c = 0;
  int prev = '\n';
  FILE* f = fopen(filename,"r");
  while(EOF != (c=fgetc(f))) {
    if ((prev=='\n') || (prev=='\r' && c!='\n')) ++nof_lines;
    prev = c;
  }
  fclose(f);
  return nof_lines;
}

char* create_fn(char* dirname, int i, char* ext)
{  char* fn = malloc((strlen(dirname)+1+nof_digits(i)+strlen(ext)+1)
		     *sizeof(char));
  sprintf(fn,"%s/%d%s", dirname,i,ext);
  return fn;
}

FILE* open_file(char* dirname, int i, char* ext, char* mode)
{
  FILE* f;
  char* fn = create_fn(dirname, i, ext);
  sprintf(fn,"%s/%d%s", dirname,i,ext);
  f = fopen(fn, mode);
  free(fn);
  return f;
}

FILE** open_files(int nof_vars, char* dirname, char* ext, char* mode)
{
  FILE** files = malloc(nof_vars * sizeof(FILE*));
  int i;
  for(i=0; i<nof_vars; ++i){
    files[i] = open_file(dirname, i, ext, mode);
  }
  return files;
}

void free_files(int nof_vars, FILE** files){
  int i;
  for(i=0; i<nof_vars; ++i) fclose(files[i]);
  free(files);
}

