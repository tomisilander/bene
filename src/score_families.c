#include "cfg.h"
#include "get_local_scores.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const char *datfile;
extern const char *essarg;
extern const char *logregfile;

extern void init_globals(const char *vdfile, const char *datfile, const char *essarg, const char *resfile,
                         const char *cstrfile, const char *priorfile, const char *logregfile,
                         const char *selfile, int use_subset_walker);
extern void free_globals(int use_subset_walker);

int main(int argc, char *argv[])
{
  const char *logreg = NULL;
  int ac = argc;
  if (ac >= 3 && !strcmp(argv[ac - 2], "-l"))
  {
    logreg = argv[ac - 1];
    ac -= 2;
  }
  if (ac != 5)
  {
    fprintf(stderr,
            "Usage: score_families vdfile datfile (BIC|AIC|...) selfile [-l logregfile]\n"
            "stdin: one line per family: child_local parent_mask (uint64, no child bit)\n");
    return 1;
  }
  const char *vd = argv[1];
  const char *dat = argv[2];
  const char *score = argv[3];
  const char *selfile = argv[4];

  datfile = dat;
  essarg = score;
  logregfile = logreg;
  init_globals(vd, dat, score, "/dev/null", NULL, NULL, logreg, selfile, 0);

  {
    int child_local;
    uint64_t mask_u64;
    while (2 == scanf("%d %" SCNu64, &child_local, &mask_u64))
    {
      varset_t parent_mask = (varset_t)mask_u64;
      score_t s = bene_score_single_family_after_init(child_local, parent_mask);
      printf("%.17g\n", (double)s);
    }
  }

  free_globals(0);
  return 0;
}
