#ifndef __CFG_H_
#define __CFG_H_
#include <float.h>

#if __STDC_VERSION__ == 199901L

#include <stdint.h>
#include <inttypes.h>
#define MAX_NOF_VARS (64)
typedef uint64_t varset_t;
#define VARSET_PRIFMT PRIu64
#define VARSET_SCNFMT SCNu64
#define LARGEST_SET(NOF_VARS) ((NOF_VARS)==MAX_NOF_VARS?(varset_t)~(uint64_t)0:((uint64_t)1<<(NOF_VARS))-1)
#else

#define MAX_NOF_VARS (32)
typedef unsigned int varset_t;
#define VARSET_PRIFMT "u"
#define VARSET_SCNFMT "u"
#define LARGEST_SET(NOF_VARS) ((NOF_VARS)==MAX_NOF_VARS?(varset_t)~0:(1U<<(NOF_VARS))-1)

#endif

#define SINGLETON(i) (((varset_t)1) << i)
#define MIN_NODE_SCORE (-FLT_MAX / MAX_NOF_VARS)
typedef double score_t;

#endif
