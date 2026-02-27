#include <stdlib.h>
#include <string.h>
#include <time.h>           /* for srand, rand */
#include "cfg.h"          /* for score_t (used in hashing) */
#include "xtab.h"

xtab* xcreate(int range, int nof_xentries){
/**
 * @brief Create and initialize a new hash table
 * 
 * @param range Number of hash buckets (hash table size)
 * @param nof_xentries Maximum number of entries the table can hold
 * 
 * @return Pointer to newly allocated and initialized xtab structure
 * 
 * @note Caller is responsible for freeing the returned pointer with xdestroy()
 */

  xtab* t     = calloc(1,sizeof(xtab));
  t->range    = range;
  t->ix       = calloc(range, sizeof(xentry*));
  t->xentries = calloc(nof_xentries, sizeof(xentry));
  t->free     = t->xentries;
  return t;
}
 
void  xdestroy(xtab* t) {
/**
 * @brief Destroy and free all resources associated with a hash table
 * 
 * @param t Pointer to the hash table to destroy
 * 
 * @note After calling this function, the pointer t should not be used
 */

  free(t->ix);
  free(t->xentries);
  free(t);
}

xentry* xadd(xtab* t, uint h, uchar* key, int klen, int* new) {

  /**
 * @brief Add or find an entry in the hash table
 * 
 * @param t Pointer to the hash table
 * @param h Hash value (index into hash table)
 * @param key Pointer to the key data
 * @param klen Length of the key in bytes
 * @param new Pointer to int flag; set to 1 if new entry was created, 0 if entry already existed
 * 
 * @return Pointer to the xentry (either newly created or existing)
 * 
 * @note Uses linear probing to resolve hash collisions
 */


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

/* ------------------------------------------------------------------
   Random hash table initializer moved from get_local_scores.c.  The
   function now takes explicit parameters instead of relying on globals
   and returns the allocated structure so that the caller can store it
   wherever is appropriate.
   ------------------------------------------------------------------ */

uint **init_xh(int nof_vars, int *nof_vals, int range)
{
  int i, v;
  uint **xh_table;

  /* deterministic behaviour for reproducibility; original code
     seeded with a fixed constant.  If desired the caller may seed
     elsewhere before invoking this function instead. */
  srand(666);

  xh_table = calloc(nof_vars, sizeof(uint *));
  if (!xh_table)
    return NULL;

  for (i = 0; i < nof_vars; ++i) {
    xh_table[i] = calloc(nof_vals[i], sizeof(uint));
    if (!xh_table[i]) {
      /* free previously allocated rows on failure */
      for (v = 0; v < i; ++v)
        free(xh_table[v]);
      free(xh_table);
      return NULL;
    }
    for (v = 0; v < nof_vals[i]; ++v) {
      xh_table[i][v] = (uint)((score_t)range * rand() / (RAND_MAX + 1.0));
    }
  }

  return xh_table;
}

/* ------------------------------------------------------------------
   Deallocate a table created by init_xh().  The implementation is very
   small but placing it in the module keeps callers from duplicating
   the two‑level free loop and centralises potential future changes.
   ------------------------------------------------------------------ */

void free_xh(int nof_vars, uint **xh_table)
{
  int i;
  if (!xh_table)
    return;

  for (i = 0; i < nof_vars; ++i)
    free(xh_table[i]);
  free(xh_table);
}

