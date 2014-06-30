#include "cfg.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "files.h"

void reverse_files(int nof_vars, char* dirname) {
  varset_t nof_parsets = 1U<<(nof_vars-1);
  score_t* buffer = malloc(nof_parsets*sizeof(score_t));

  int v;
  for(v=0; v<nof_vars; ++v){

    {
      FILE* fin  = open_file(dirname, v, ".slt", "rb");
      FREAD(buffer, sizeof(score_t), nof_parsets, fin);
      fclose(fin);
    }

    {
      FILE* fout = open_file(dirname, v, "", "wb");
      varset_t ps = nof_parsets;
      do {
	fwrite(buffer + (--ps), sizeof(score_t), 1, fout);
      } while(ps);
      
      fclose(fout);
    }
  }
  
  free(buffer);

}


int main(int argc, char* argv[])
{
  if (argc!=3) {
    fprintf(stderr, "Usage: reverse_files nof_vars dirname\n");
    return 1;
  }

  {
    int nof_vars = atoi(argv[1]);
    reverse_files(nof_vars, argv[2]);
  }

  return 0;
}
