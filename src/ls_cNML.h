#ifndef _LS_CNML_H_
#define _LS_CNML_H_

#include "cfg.h"
#include "get_local_scores.h"

/* NML = log(ML) - SUM_D log(ML(D)) */

extern scorefun init_cNML_scorer();
extern void free_cNML_scorer();

extern score_t* cnml_scoretable; /* nml probabilities for data column subsets */
extern scorefun cnml_vs_scorer;

#endif
