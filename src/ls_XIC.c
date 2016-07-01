#include <math.h>
#include <string.h>
#include "get_local_scores.h"
#include "ilogi.h"
#include "ls_XIC.h"


extern int*     nof_vals;
extern int      N;

static int bic = 0;

#define BIG_XIC_DATA (1<<16)
int nof_ilogi = 0; 

/* XIC = log(ML) - 0.5*nof_params*log(n) */

score_t xic_score(int i, varset_t psi, int nof_freqs){

  int vc_v = nof_vals[i];
  score_t pcc =  get_nof_cfgs(psi);
  score_t nof_params = pcc * (vc_v-1);
  int* freqp = freqmem;
  int* end_freqp = freqp + nof_freqs * vc_v;
  score_t res = bic ? (-0.5 * nof_params * log(N)) : -nof_params;

  
  for(;freqp < end_freqp; freqp += vc_v) {
    int  pcfreq = 0;
    int v;
    for(v = 0; v<vc_v; ++v) {
      int freq = freqp[v];
      if (freq) {
	pcfreq += freq;
	res += (freq < nof_ilogi) ? ilogi[freq] : freq * log(freq);
      }
    }
    res -= (pcfreq < nof_ilogi) ? ilogi[pcfreq] : pcfreq * log(pcfreq);

  }

  return res;
}

scorefun init_XIC_scorer(char* essarg){
  bic = strncmp(essarg,"BIC", 3) == 0;
  nof_ilogi = MIN(N,BIG_XIC_DATA); 
  ensure_ilogi(nof_ilogi);
  return xic_score;
}

void free_XIC_scorer(){
  free_ilogi();
}
