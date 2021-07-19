#include "cfg.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "files.h"

void get_best_sinks(int nof_vars, FILE** files, char* sinkfile)
{

  FILE* sinkf        = fopen(sinkfile, "wb");
  score_t* scores    = malloc((1U<<nof_vars) * sizeof(score_t));
  score_t best_score = 0.0;

  varset_t varset;
  for(varset=0; varset < (1U<<nof_vars); ++varset){
    int sink;
    char best_sink = -1;

    for(sink=0; sink<nof_vars; ++sink){ /* could use ffs */
      varset_t sinkleton = 1U<<sink;

      if (varset & sinkleton) {
	score_t new_score = scores[varset & ~sinkleton];
	score_t par_score;
	FREAD(&par_score, sizeof(score_t), 1, files[sink]);
	new_score += par_score;
	if (best_sink == -1 || best_score < new_score){
	  best_score = new_score;
	  best_sink  = sink;
	}
      }
    }
    
    scores[varset] = best_score;
    fwrite(&best_sink, sizeof(char), 1, sinkf);
  }

  free(scores);
  fclose(sinkf);
}


int main(int argc, char* argv[])
{

  if (argc!=4) {
    fprintf(stderr, "Usage: get_best_net nof_vars dirname sinkfile\n");
    return 1;
  }
  
  {
    int nof_vars = atoi(argv[1]);
    FILE** files = open_files(nof_vars, argv[2],".bss", "rb");
    get_best_sinks(nof_vars, files, argv[3]);
    free_files(nof_vars, files);
  }
  
  return 0;
}
