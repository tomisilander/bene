#include <math.h>
#include <string.h>
#include "get_local_scores.h"
#include "ilogi.h"
#include "ls_XIC.h"


extern int*     nof_vals;
extern int      N;

static char xic = 0;
static score_t log_N = 0.0;
static score_t loglog_N = 0.0;

#define BIG_XIC_DATA (1<<16)

score_t xic_score(int i, varset_t psi, int nof_freqs){

  int vc_v = nof_vals[i];
  score_t pcc =  get_nof_cfgs(psi);
  score_t nof_params = pcc * (vc_v-1);
  int* freqp = freqmem;
  int* end_freqp = freqp + nof_freqs * vc_v;

  score_t res = 0.0;
  switch (xic) {
  case 'B':
    res = -0.5 * nof_params * log_N;
    break;
  case 'A':
    res = -nof_params;
    break;
  case 'H':
    res = -nof_params * loglog_N;
  }
  
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

  }

  return res;
}

scorefun init_XIC_scorer(const char* essarg){
  xic = essarg[0]; /* B, A or H */ 
  ensure_ilogi(MIN(N,BIG_XIC_DATA));
  log_N = log(N);
  loglog_N = log(log_N);
  return xic_score;
}

void free_XIC_scorer(){
  free_ilogi();
}
