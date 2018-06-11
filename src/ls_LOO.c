#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "get_local_scores.h"
#include "ls_LOO.h"

extern int*     nof_vals;
extern int      N;

static score_t ess;
static int splitess;

static int*    freq2mem  = NULL;
static int*    freq2mem2 = NULL;
static int     nof_freq2s;


#define BIG_LOO_DATA (1<<10)

/* LOO */

score_t big_loo_score(int i, varset_t psi, int nof_freqs){

  int vc_v = nof_vals[i];
  score_t pcc =  get_nof_cfgs(psi);
  score_t ess_per_pcc = splitess ? (ess / pcc)          : (ess*vc_v);
  score_t ess_per_cc  = splitess ? (ess_per_pcc / vc_v) : ess;

  int* freqp = freqmem;
  int* end_freqp = freqp + nof_freqs * vc_v;

  score_t res = - log(N);
    
  memset(freq2mem,  0, nof_freq2s*sizeof(int)); /* could reset later */
  memset(freq2mem2, 0, nof_freq2s*sizeof(int)); /* could reset later */

  for(;freqp < end_freqp; freqp += vc_v) {
    int  pcfreq = 0;
    int v;
    for(v = 0; v<vc_v; ++v) {
      int freq = freqp[v];
      if (freq) {
	pcfreq += freq;
	if(freq<nof_freq2s){
	  ++freq2mem[freq];
	} else {
	  res += freq * log(ess_per_cc + freq - 1);
	}
      }
    }

    if(pcfreq<nof_freq2s){
      ++freq2mem2[pcfreq];
    } else {
      res -= pcfreq * log(ess_per_pcc + pcfreq);
    }
  }

  {
    int i;

    for(i=1;i<nof_freq2s;++i){
      int freq2 = freq2mem[i];
      if(freq2) res += freq2 * i * log(ess_per_cc + i - 1);
    }

    for(i=1;i<nof_freq2s;++i){
      int freq2 = freq2mem2[i];
      if(freq2) res -= freq2 * i * log(ess_per_pcc + i);
    }

  }

  return res > 0.0 ? 0.0 : res;
}


score_t loo_score(int i, varset_t psi, int nof_freqs){

  int vc_v = nof_vals[i];
  score_t pcc =  get_nof_cfgs(psi);

  score_t ess_per_pcc = splitess ? (ess / pcc)          : (ess*vc_v);
  score_t ess_per_cc  = splitess ? (ess_per_pcc / vc_v) : ess;

  int* freqp = freqmem;
  int* end_freqp = freqp + nof_freqs * vc_v;

  score_t res = -log(N);
    
  memset(freq2mem,  0, nof_freq2s*sizeof(int)); /* could reset later */
  memset(freq2mem2, 0, nof_freq2s*sizeof(int)); /* could reset later */

  for(;freqp < end_freqp; freqp += vc_v) {
    int  pcfreq = 0;
    int v;
    for(v = 0; v<vc_v; ++v) {
      int freq = freqp[v];
      if (freq) {
	pcfreq += freq;
	++freq2mem[freq];
      }
    }
    ++freq2mem2[pcfreq];
  }

  {
    int i;

    for(i=1;i<nof_freq2s;++i){
      int freq2 = freq2mem[i];
      if(freq2) res += freq2 * i * log(ess_per_cc + i - 1);
    }

    for(i=1;i<nof_freq2s;++i){
      int freq2 = freq2mem2[i];
      if(freq2) res -= freq2 * i * log(ess_per_pcc + i);
    }

  }

  return res > 0.0 ? 0.0 : res;
}



scorefun init_LOO_scorer(char* arg){

  scorefun sf;
  int arglen = strlen(arg);

  ess = atof(arg);
  splitess = arg[arglen-1] == 'l';

  if (N<BIG_LOO_DATA){
    sf = loo_score;
    nof_freq2s = N+1;
  } else {
    sf = big_loo_score;
    nof_freq2s = BIG_LOO_DATA;
  }

  freq2mem  = malloc(nof_freq2s * sizeof(int));
  freq2mem2 = malloc(nof_freq2s * sizeof(int));

  return sf;
}

void free_LOO_scorer(){
  if(freq2mem)  free(freq2mem);
  if(freq2mem2) free(freq2mem2);
}
