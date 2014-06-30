#include "cfg.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "files.h"


void force_arc(int nof_vars, char* dirname, int src, int dst) {

  varset_t nof_parsets = 1U<<(nof_vars-1);
  score_t*  scores = malloc(nof_parsets*sizeof(score_t));
  
  { /* READ SCORES FOR dst */
    FILE* fin = open_file(dirname, dst, "", "rb");
    FREAD(scores, sizeof(score_t), nof_parsets, fin);
    fclose(fin);
  }

  { /* REPLACE SCORES THAT HAVE SRC IN THE PARENT SET */

    varset_t ps;
    varset_t srcset = (src<dst) ? (1U<<src) : (1U<<(src-1));
    score_t* sp = scores;
    for(ps=0; ps<nof_parsets; ++ps, ++sp){
      if (!(srcset & ps)) {
	*sp = (score_t) MIN_NODE_SCORE;
      }
    }
  }
    
  { /* WRITE SCORES TO A BAN FILE */
    
    FILE* fout = open_file(dirname, dst, ".ban", "wb");
    fwrite(scores, sizeof(score_t), nof_parsets, fout);
    fclose(fout);
  }

  free(scores);
}


int main(int argc, char* argv[])
{

  if (argc!=5) {
    fprintf(stderr, "Usage: force_arc nof_vars src dst dirname\n");
    return 1;
  }

  {
    int nof_vars = atoi(argv[1]);
    int src = atoi(argv[2]);
    int dst = atoi(argv[3]);
    force_arc(nof_vars, argv[4], src, dst);
  }

  return 0;
}
