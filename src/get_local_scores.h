#ifndef _GET_LOCAL_SCORES_H_
#define _GET_LOCAL_SCORES_H_

#include "cfg.h"

extern int  N;
extern int  nof_vars;
extern int* nof_vals;
extern int*    freqmem; /* a working memory to collect the conditional frequences */

extern score_t get_nof_cfgs(varset_t vs);
typedef score_t (*scorefun) (int, varset_t, int);
typedef void (*scorefree) ();

#endif
