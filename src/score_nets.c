#include "cfg.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "files.h"
#include "varpar.h"

score_t score_net(char* netfile, char* dirprefix, int nof_vars, FILE** fins){
  score_t score = 0.0;

  int v;
  varset_t varset;
  FILE* netf = strcmp(netfile,"-") ? fopen(netfile, "r") : stdin;
  for(v=0; 1 == fscanf(netf, "%"VARSET_SCNFMT, &varset); ++v){
    score_t vscore;
    fseek(fins[v], varset2parset(v, varset)*sizeof(score_t), SEEK_SET);
    FREAD(&vscore, sizeof(score_t), 1, fins[v]);
    rewind(fins[v]);
    score += vscore;
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
    fprintf(stderr, "Usage: score_nets netfiles dirname\n");
    return 1;
  }

  score_nets(argv[1], argv[2]);

  return 0;
}
