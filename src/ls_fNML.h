#ifndef _LS_NML_H_
#define _LS_NML_H_

#include "cfg.h"
#include "get_local_scores.h"

/* NML = log(ML) - SUM_D log(ML(D)) */

extern scorefun init_fNML_scorer();
extern score_t fnml_score();
extern void free_fNML_scorer();

#endif
