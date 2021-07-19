#include "cfg.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFSIZE (1024)

void parents2arcs(char* parfile, char* arcfile)
{
  char buffer[BUFSIZE];
  int v;
  FILE* parf = strcmp(parfile,"-") ? fopen(parfile, "r") : stdin;
  FILE* arcf = strcmp(arcfile,"-") ? fopen(arcfile, "w") : stdout;
  for(v=0; NULL != fgets(buffer, BUFSIZE, parf); ++v){
    /* could check that the last character in buffer is newline */
    char* token;
    for(token=strtok(buffer, " \t\r\n"); token; token=strtok(NULL, " \t\r\n")){
      fprintf(arcf, "%d %d\n", atoi(token), v);
    }
  }
  fclose(arcf);
  fclose(parf);
}

int main(int argc, char* argv[])
{

  if (argc!=3) {
    fprintf(stderr, "Usage: parents2arcs parentfile arcfile\n");
    return 1;
  }

  parents2arcs(argv[1], argv[2]);

  return 0;
}
