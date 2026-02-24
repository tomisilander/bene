/**
 * @file xtab.h
 * @brief Hash table implementation for key-value storage with byte array keys and pointer values.
 *
 * This module provides a hash table data structure that maps byte array keys to pointer values.
 * It uses separate chaining for collision resolution and maintains a pool of pre-allocated
 * entries for efficient memory management since maximum number of entries is known at creation.
 */

#ifndef _XTAB_H_
#define _XTAB_H_

#include <stdlib.h>
#include <string.h>

typedef struct xentry xentry;
typedef unsigned char uchar;
typedef unsigned int uint;

/**
 * @struct xentry
 * @brief A single entry in the hash table.
 * @var xentry::key
 *   Pointer to the key string (null-terminated).
 * @var xentry::val
 *   Pointer to the associated integer value.
 * @var xentry::next
 *   Pointer to the next entry in the collision chain.
 * @var xentry::h
 *   Precomputed hash value for the key.
 */

 struct xentry {
  uchar* key;
  int*  val;
  xentry* next;
  uint h;       /* h denotes hash key */
};

typedef struct xtab xtab;

/**
 * @struct xtab
 * @brief Main hash table structure.
 * @var xtab::range
 *   Size of the hash table (number of buckets).
 * @var xtab::ix
 *   Array of pointers to hash table buckets (collision chains).
 * @var xtab::free
 *   Pointer to the next available unused entry in the entry pool.
 * @var xtab::xentries
 *   Array of pre-allocated hash table entries.
 */

 struct xtab {
  int range;        /* range of hash keys, for example 1024 */
  xentry** ix;      /* hashkey to xentry pointer mapping    */
  xentry* free;     /* pointer to unused xentries           */
  xentry* xentries;
};

/**
 * @def xcount(t)
 * @brief Macro to get the number of entries currently in use.
 * @param t Pointer to the hash table.
 * @return Number of used entries.
 */

 #define xcount(t) ((t)->free - (t)->xentries)

/**
 * @def xreset(t)
 * @brief Macro to reset the hash table to empty state.
 * Clears all buckets and reinitializes the entry pool.
 * @param t Pointer to the hash table to reset.
 */

 #define xreset(t) \
{\
  memset((t)->ix, 0, (t)->range * sizeof(xentry*));\
  memset((t)->xentries, 0, xcount(t) * sizeof(xentry));\
  (t)->free = (t)->xentries;\
}

/**
 * @brief Create and initialize a new hash table.
 * @param range Number of buckets in the hash table.
 * @param nof_xentries Maximum number of entries to allocate.
 * @return Pointer to newly created hash table, or NULL on allocation failure.
 */

 extern xtab* xcreate(int range, int nof_xentries);

/**
 * @brief Destroy and free a hash table.
 * @param t Pointer to the hash table to destroy.
 */

 extern void  xdestroy(xtab* t);

/**
 * @brief Add or retrieve an entry in the hash table.
 * @param t Pointer to the hash table.
 * @param h Precomputed hash value.
 * @param key Pointer to the key string.
 * @param klen Length of the key string.
 * @param new Pointer to integer flag (set to 1 if new entry was created, 0 if existing).
 * @return Pointer to the hash table entry, or NULL if no space available.
 */

 extern xentry* xadd(xtab* t, uint h, uchar* key, int klen, int* new);

#endif
