#ifndef _XTAB_H_
#define _XTAB_H_

#include <string.h>

typedef struct xentry xentry;
typedef unsigned char uchar;
typedef unsigned int uint;

struct xentry {
  uchar* key;
  int*  val;
  xentry* next;
  uint h;       /* h denotes hash key */
};

typedef struct xtab xtab;

struct xtab {
  int range;        /* range of hash keys, for example 1024 */
  xentry** ix;      /* hashkey to xentry pointer mapping    */
  xentry* free;     /* pointer to unused xentries           */
  xentry* xentries;
};


#define xcount(t) ((t)->free - (t)->xentries)

# define xreset(t) \
{\
  memset((t)->ix, 0, (t)->range * sizeof(xentry*));\
  memset((t)->xentries, 0, xcount(t) * sizeof(xentry));\
  (t)->free = (t)->xentries;\
}

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

#endif
