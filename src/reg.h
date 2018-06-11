#ifndef __REG_H__
#define __REG_H__
#include "cfg.h"


extern void init_logreg(const char* logregfile, int nof_vars, int* nofvals);
extern float logreg(int N, int K);
extern void free_logreg(int nof_vars, int* nof_vals);

#endif
