#include "cfg.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "files.h"


void zeta_local_scores(int nof_vars, char* dirname) {

  varset_t nof_parsets = 1U<<(nof_vars-1);
  score_t* scores      = malloc(nof_parsets*sizeof(score_t));
  score_t  max_score   = MIN_NODE_SCORE;

  int i;
  for(i=0;i<nof_vars; ++i){
    
    { /* READ SCORES FOR i */
      FILE* fin = open_file(dirname, i, "", "rb");
      FREAD(scores, sizeof(score_t), nof_parsets, fin);
      fclose(fin);
    }

    { /* FIND MAX SCORE AND THEN SCALE AND EXP*/
      varset_t ps;
      for(ps=0; ps<nof_parsets; ++ps) {
	if (scores[ps] > max_score) max_score = scores[ps];
      }
      for(ps=0; ps<nof_parsets; ++ps){
	scores[ps] = exp(scores[ps] - max_score);
      }
    }    
    
    { /* ZETA TRANSFORM THE SCORES */

      varset_t ps;
      int j;

      for(j=1; j<=nof_vars;++j) {
	varset_t jin  = 1<<j;
	varset_t jout = ~jin;

	for(ps=0; ps<nof_parsets; ++ps)
	  if (ps & jin) scores[ps] += scores[ps&jout];
      }
    }
    
    
    { /* LOG THEM BACK */
      varset_t ps;
      for(ps=0; ps<nof_parsets; ++ps){
	scores[ps] = log(scores[ps]) + max_score;
      }
    }    

    { /* WRITE SCORES FOR i */

      FILE* fout = open_file(dirname, i, "", "wb");
      fwrite(scores, sizeof(score_t), nof_parsets, fout);
      fclose(fout);
    }
  }

  free(scores);
}


int main(int argc, char* argv[])
{

  if (argc!=3) {
    fprintf(stderr, "Usage: zeta_local_scores nof_vars dirname\n");
    return 1;
  }

  {
    int nof_vars = atoi(argv[1]);
    zeta_local_scores(nof_vars, argv[2]);
  }

  return 0;
}
