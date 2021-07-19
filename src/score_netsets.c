#include "cfg.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "files.h"
#include "varpar.h"

score_t score_net(char* netfile, char* dirprefix, int nof_vars, FILE** fins){
  score_t score = 0.0;

  int v;
  varset_t varset;
  FILE* netf = strcmp(netfile,"-") ? fopen(netfile, "r") : stdin;
  for(v=0; 1 == fscanf(netf, "%"VARSET_SCNFMT, &varset); ++v){ /* for each variable */
    score_t vscore  = 0; /* sum the wscores here */ 
    score_t pscore  = 0; /* original parent set score to used as a scaler */
    score_t wscore  = 0; /* working parent set score */
    varset_t parset = varset2parset(v, varset);
    varset_t warset = parset; /* a working copy of parset */
    
    fseek(fins[v], parset*sizeof(score_t), SEEK_SET);
    FREAD(&pscore, sizeof(score_t), 1, fins[v]);
    rewind(fins[v]);

    while (1) { /* for each parent set */
	fseek(fins[v], warset*sizeof(score_t), SEEK_SET);
	FREAD(&wscore, sizeof(score_t), 1, fins[v]);
	rewind(fins[v]);
	
	vscore += exp(wscore-pscore);

	if (warset == 0) break;
	warset = (warset-1)&parset;
      }
    score += log(vscore)+pscore;
  }
  fclose(netf);
  return score;
}

void score_nets(char* netfiles, char* dirprefix){
  char netfile[1024];
  FILE* netfs = strcmp(netfiles,"-") ? fopen(netfiles, "r") : stdin;
  int nof_vars = 0;
  FILE** fins = NULL; 
  while(EOF != fscanf(netfs, "%s", netfile)){
    if (0 == nof_vars){
      nof_vars = nof_lines(netfile);
      fins = open_files(nof_vars, dirprefix,"","r");
    }
    printf("%.3f\n", score_net(netfile, dirprefix, nof_vars, fins));
  }
  free_files(nof_vars,fins);
  fclose(netfs);
}

int main(int argc, char* argv[])
{

  if (argc!=3) {
    fprintf(stderr, "Usage: score_netsets netfiles dirname\n");
    return 1;
  }

  score_nets(argv[1], argv[2]);

  return 0;
}
