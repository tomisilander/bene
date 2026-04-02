#include "cfg.h"
#include "files.h"
#include "varpar.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    fprintf(stderr, "Usage: net_local_scores netfile dirname\n");
    return 1;
  }
  int v;
  varset_t varset;
  FILE *netf = strcmp(argv[1], "-") ? fopen(argv[1], "r") : stdin;
  if (!netf)
  {
    fprintf(stderr, "net_local_scores: cannot open netfile\n");
    return 1;
  }
  printf("[");
  int first = 1;
  for (v = 0; 1 == fscanf(netf, "%" VARSET_SCNFMT, &varset); ++v)
  {
    FILE *fin = open_file(argv[2], v, "", "rb");
    fseek(fin, varset2parset(v, varset) * sizeof(score_t), SEEK_SET);
    score_t vscore;
    FREAD(&vscore, sizeof(score_t), 1, fin);
    fclose(fin);
    if (!first)
      printf(",");
    first = 0;
    printf("{\"node\":%d,\"parent_set\":%" PRIu64 ",\"score\":%.17g}", v, (uint64_t)varset,
           (double)vscore);
  }
  printf("]\n");
  fclose(netf);
  return 0;
}
