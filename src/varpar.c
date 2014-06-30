#include "varpar.h"

varset_t parset2varset(int v, varset_t set){
  varset_t sinkleton = 1U<<v;
  varset_t mask      = sinkleton - 1;
  varset_t low       = set &  mask;
  varset_t high      = set & ~mask;
  return (high<<1) | low;
}

varset_t varset2parset(int v, varset_t set){
  /* assumes v not set */
  varset_t sinkleton = 1U<<v;
  varset_t mask      = sinkleton - 1;
  varset_t low       = set &  mask;
  varset_t high      = set & ~mask;
  return (high>>1) | low;
}

