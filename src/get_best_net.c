#include "cfg.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "files.h"
#include "varpar.h"

void get_best_net(int nof_vars, char* ordfile, FILE** bpsfiles, varset_t* net)
{
  FILE* ordf = fopen(ordfile,"r");
  varset_t parcands = 0;
  int i;
  for(i=0;i<nof_vars; ++i){
    varset_t parset;
    int v;
    (void) (1 == fscanf(ordf, "%d", &v));
    fseek(bpsfiles[v], varset2parset(v,parcands)*sizeof(varset_t), SEEK_SET);
    FREAD(&parset, sizeof(varset_t), 1, bpsfiles[v]);
    parcands |= 1U<<v;
    net[v] = parset2varset(v, parset);
  }
  fclose(ordf);
}


int main(int argc, char* argv[])
{

  if (argc!=5) {
    fprintf(stderr, "Usage: get_best_net nof_vars dirname ordfile netfile\n");
    return 1;
  }
  
  {
    int nof_vars = atoi(argv[1]);

    varset_t* net    = malloc(nof_vars*sizeof(varset_t));
    FILE** files = open_files(nof_vars, argv[2],".bps", "rb");
    get_best_net(nof_vars, argv[3], files, net);
    free_files(nof_vars, files);

    {
      FILE* netf = fopen(argv[4],"w");
      int i;
      for (i=0;i<nof_vars;++i){
	fprintf(netf, "%"VARSET_PRIFMT"\n", net[i]);
      }
      fclose(netf);
    }

    free(net);
  }
    
  return 0;
}
