#include <stdlib.h>
#include <string.h>
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

