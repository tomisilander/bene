#include "cfg.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFSIZE (1024)

void arcs2dot(char* vdfile, char* arcfile, char* dotfile)
{
  char buffer[BUFSIZE];
  FILE* dotf = strcmp(dotfile,"-") ? fopen(dotfile, "w") : stdout;

  fprintf(dotf, "digraph Bene {\n");
  fprintf(dotf, "  /*@@@BEFORE_ALL@@@*/\n"); 
  
  {/* read vdfile to get names of variables */

    FILE* vdf  = fopen(vdfile, "r");
    int v;
    fprintf(dotf, "  /*@@@BEFORE_NODES@@@*/\n"); 
    for(v=0; NULL != fgets(buffer, BUFSIZE, vdf); ++v){
      char* token = strtok(buffer, "\t\r\n");
      fprintf(dotf, "  V%d [label=\"%s\" /*@@@NODE_ATTS@@@*/];\n", v, token);
    }
    fprintf(dotf, "  /*@@@AFTER_NODES@@@*/\n"); 
    fclose(vdf);
  }

  {

    int from, to;
    FILE* arcf = strcmp(arcfile,"-") ? fopen(arcfile, "r") : stdin;

    fprintf(dotf, "  /*@@@BEFORE_EDGES@@@*/\n"); 
    while(2 == fscanf(arcf, "%d %d", &from, &to)){
      fprintf(dotf, "  V%d -> V%d /*@@@EDGE_ATTS@@@*/;\n", from, to);
    }
    fprintf(dotf, "  /*@@@AFTER_EDGES@@@*/\n"); 

    fclose(arcf);
  }
  
  fprintf(dotf, "  /*@@@AFTER_ALL@@@*/\n"); 
  fprintf(dotf, "}\n"); 
  fclose(dotf);
}

int main(int argc, char* argv[])
{

  if (argc!=4) {
    fprintf(stderr, "Usage: arcs2dot vdfile arcfile dotfile\n");
    return 1;
  }

  arcs2dot(argv[1], argv[2], argv[3]);

  return 0;
}
