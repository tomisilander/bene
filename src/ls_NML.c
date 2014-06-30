#include <math.h>
#include "get_local_scores.h"
#include "ilogi.h"
#include "reg.h"
#include "ls_NML.h"

extern int*     nof_vals;
extern int      N;

#define BIG_NML_DATA (1<<16)

/* NML = log(ML) - log(REGRET) */

score_t big_nml_score(int i, varset_t psi, int nof_freqs){

  int vc_v = nof_vals[i];
  int* freqp = freqmem;
  int* end_freqp = freqp + nof_freqs * vc_v;

  score_t res = 0.0;

  for(;freqp < end_freqp; freqp += vc_v) {
    int  pcfreq = 0;
    int v;
    for(v = 0; v<vc_v; ++v) {
      int freq = freqp[v];
      if (freq) {
	pcfreq += freq;
	res += (freq < BIG_NML_DATA) ? ilogi[freq] : freq * log(freq);
      }
    }
    res -= (pcfreq < BIG_NML_DATA) ? ilogi[pcfreq] : pcfreq * log(pcfreq);
    res -= log(reg(pcfreq, vc_v)); /* regret */
  }

  return res;
}

score_t nml_score(int i, varset_t psi, int nof_freqs){

  int vc_v = nof_vals[i];
  int* freqp = freqmem;
  int* end_freqp = freqp + nof_freqs * vc_v;

  score_t res = 0.0;

  for(;freqp < end_freqp; freqp += vc_v) {
    int  pcfreq = 0;
    int v;
    for(v = 0; v<vc_v; ++v) {
      int freq = freqp[v];
      if (freq) {
	pcfreq += freq;
	res += ilogi[freq];
      }
    }
    res -= ilogi[pcfreq];
    res -= log(reg(pcfreq, vc_v)); /* regret */
  }

  return res;
}


scorefun init_NML_scorer(){
  if (N<BIG_NML_DATA){
    ensure_ilogi(N);
    return nml_score;
  } else {
    ensure_ilogi(BIG_NML_DATA);
    return big_nml_score;
  }
}

void free_NML_scorer(){
  free_ilogi();
}
