#ifndef _LS_XIC_H_
#define _LS_XIC_H_

#include "cfg.h"
#include "get_local_scores.h"

/* BIC = log(ML) - 0.5*nof_params*log(n) */
/* AIC = log(ML) - 0.5*nof_params */

extern scorefun init_XIC_scorer(const char*);
extern void free_XIC_scorer();

#endif
