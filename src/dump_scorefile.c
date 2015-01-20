#include "cfg.h"
#include <stdio.h>


void dump_file(const char* filename) {
  FILE* fin  = fopen(filename, "rb");
  score_t score = 0;
  while (fread(&score, sizeof(score_t), 1, fin)) printf("%f\n",score);
  fclose(fin);
}


int main(int argc, char* argv[])
{
  if (argc!=2) {
    fprintf(stderr, "Usage: dump_scorefile filename\n");
    return 1;
  }

  dump_file(argv[1]);

  return 0;
}
