#ifndef _LS_CNML_H_
#define _LS_CNML_H_

#include "cfg.h"
#include "get_local_scores.h"

extern scorefun init_qNML_scorer();
extern void free_qNML_scorer();

extern score_t* qnml_scoretable; /* nml probabilities for data column subsets */
extern scorefun qnml_vs_scorer;

#endif
