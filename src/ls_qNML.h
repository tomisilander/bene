#ifndef _LS_QNML_H_
#define _LS_QNML_H_

#include "cfg.h"
#include "get_local_scores.h"

extern scorefun init_qNML_scorer();
extern void free_qNML_scorer();
extern scorefun qnml_vs_scorer;

#endif
