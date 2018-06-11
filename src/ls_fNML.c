#include <math.h>
#include "get_local_scores.h"
#include "ilogi.h"
#include "reg.h"
#include "ls_fNML.h"

extern int*     nof_vals;
extern int      N;

#define BIG_NML_DATA (1<<16)

/* NML = log(ML) - log(REGRET) */

score_t fnml_score(int i, varset_t psi, int nof_freqs){

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
	res += (freq < len_ilogi) ? ilogi[freq] : freq * log(freq);
      }
    }
    res -= (pcfreq < len_ilogi) ? ilogi[pcfreq] : pcfreq * log(pcfreq);
    res -= logreg(pcfreq, vc_v); /* regret */
  }

  return res;
}

scorefun init_fNML_scorer(const char* logregfile){
  init_logreg(logregfile, nof_vars, nof_vals);
  ensure_ilogi(MIN(N,BIG_NML_DATA));
  return fnml_score;
}

void free_fNML_scorer(){
  free_ilogi();
  free_logreg(nof_vars, nof_vals);
}
