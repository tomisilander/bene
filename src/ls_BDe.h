#ifndef _LS_BDE_H_
#define _LS_BDE_H_

#include "cfg.h"
#include "get_local_scores.h"

/* BDe = P(D|G,ess) */

extern scorefun init_BDe_scorer();
extern void free_BDe_scorer();

#endif
