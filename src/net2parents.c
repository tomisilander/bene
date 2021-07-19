#include "cfg.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void net2parents(char* netfile, char* parfile)
{
  int v;
  varset_t varset;
  FILE* netf = strcmp(netfile,"-") ? fopen(netfile, "r") : stdin;
  FILE* parf = strcmp(parfile,"-") ? fopen(parfile, "w") : stdout;
  for(v=0; 1 == fscanf(netf, "%"VARSET_SCNFMT, &varset); ++v){
    int p;
    for(p=0; varset; ++p, varset>>=1){
      if(varset & 1) {
	fprintf(parf, "%d", p);
	if(varset>1)fprintf(parf, " ");
      }
    }
    fprintf(parf,"\n");
  }
  fclose(parf);
  fclose(netf);
}

int main(int argc, char* argv[])
{

  if (argc!=3) {
    fprintf(stderr, "Usage: net2parents netfile parentfile\n");
    return 1;
  }

  net2parents(argv[1], argv[2]);

  return 0;
}
