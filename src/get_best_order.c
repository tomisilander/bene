#include "cfg.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "files.h"

void get_best_order(int nof_vars, char* sinkfile, char* ord)
{
  FILE* sinkf = fopen(sinkfile, "rb");
  varset_t  set = LARGEST_SET(nof_vars);
  int i;
  for(i=nof_vars-1; i>=0;--i){
    char sink;

    fseek(sinkf, set, SEEK_SET);
    FREAD(&sink, sizeof(char), 1, sinkf);
    ord[i] = sink;

    set ^= 1U<<sink;
  }
  fclose(sinkf);
}


int main(int argc, char* argv[])
{

  if (argc!=4) {
    fprintf(stderr, "Usage: get_best_net nof_vars sinkfile ordfile\n");
    return 1;
  }

  {
    int nof_vars = atoi(argv[1]);
    char* ord = malloc(nof_vars*sizeof(char));

    get_best_order(nof_vars, argv[2], ord);

    {
      FILE* ordf  = fopen(argv[3],  "w");
      int i;
      for(i=0;i<nof_vars;++i){
	fprintf(ordf, "%d%c", ord[i], i==nof_vars-1 ? '\n' : ' ');
      }
      fclose(ordf);
    }
  }

  return 0;
}
