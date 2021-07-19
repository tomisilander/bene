#include <stdlib.h>
#include <math.h>
#include "cfg.h"
#include "ilogi.h"

score_t* ilogi = NULL;
int      len_ilogi = 0;

static int refcount = 0;

int ensure_ilogi(int maxi){
  if (maxi >= len_ilogi)
    len_ilogi = maxi+1;
  
  ilogi = (score_t*) realloc(ilogi, len_ilogi * sizeof(score_t));
  ++refcount;

  {
    int i;

    if (ilogi) 
      ilogi[0] = 0.0;

    for(i=1;i<len_ilogi;++i) 
      ilogi[i] = i*log(i);
  }
  
  return 1;
}
 
void free_ilogi(){
  --refcount;
  if (!refcount){
    free(ilogi);
    ilogi = NULL;
  }
}
