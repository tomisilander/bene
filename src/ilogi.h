#ifndef _ILOGI_H_
#define _ILOGI_H_
#include "cfg.h"

extern score_t* ilogi;
extern int ensure_ilogi(int maxi);
extern void free_ilogi();

#define MIN(X,Y) (X) ? ((X)<(Y)) : (Y)

#endif
