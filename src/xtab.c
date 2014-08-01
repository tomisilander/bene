#include <stdlib.h>
#include <string.h>
#include "xtab.h"


xentry* xadd(xtab* t, uint h, uchar* key, int klen, int* new) {

  /* starting from ix[h], go through x:s and try to find key */

  xentry* x = t->ix[h];
  xentry** p = t->ix + h; /* prev-pointer to chain the possibly new xentry */
  for(; x && memcmp(x->key, key, klen); x = x->next)
    p = &(x->next);

  *new = !x;

  if(*new){
    x = (t->free)++;
    x->h = h;
    *p = x; /* link the new x */
  }

  return x;
}

xtab* xcreate(int range, int nof_xentries){
  xtab* t     = calloc(1,sizeof(xtab));
  t->range    = range;
  t->ix       = calloc(range, sizeof(xentry*));
  t->xentries = calloc(nof_xentries, sizeof(xentry));
  t->free     = t->xentries;
  return t;
}
 
void  xdestroy(xtab* t) {
  free(t->ix);
  free(t->xentries);
  free(t);
}

