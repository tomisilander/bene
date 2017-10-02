#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "get_local_scores.h"
#include "ilogi.h"
#include "ls_BDq.h"

extern int*     nof_vals;
extern int      N;

score_t ess;

static int*    freq2mem  = NULL;
static int*    freq2mem2 = NULL;
static int     nof_freq2s;

#define BIG_BDE_DATA (1<<10)

static int data_not_big = 0;

extern double lgamma(double); /* to avoid the warning with ansi-flag */

/* BDe */

score_t bdq_score(int i, varset_t psi, int nof_freqs){

  int vc_v = nof_vals[i];
  score_t pcc =  get_nof_cfgs(psi);
  score_t qess  = pcc*ess;
  score_t qress = qess*vc_v;

  int* freqp = freqmem;
  int* end_freqp = freqp + nof_freqs * vc_v;

  score_t res = lgamma(qress)+lgamma(N+qess)-lgamma(qess)-lgamma(N+qress);
    
  memset(freq2mem,  0, nof_freq2s*sizeof(int)); 
  memset(freq2mem2, 0, nof_freq2s*sizeof(int)); 

  /* in order to limit calls to lgamma, we store the small counts to freq2mem */

  for(;freqp < end_freqp; freqp += vc_v) {
    int  pcfreq = 0;
    int v;
    for(v = 0; v<vc_v; ++v) {
      int freq = freqp[v];
      pcfreq += freq;
      if(data_not_big || freq<nof_freq2s){
	++freq2mem[freq];
      } else {
	res += lgamma(ess + freq);
      }
    }

    /* We store the small parent config counts to freq2mem2 */

    if(data_not_big || pcfreq<nof_freq2s){
      ++freq2mem2[pcfreq];
    } else {
      res -= lgamma(ess + pcfreq);
    }
  }

  freq2mem2[0] += pcc*(vc_v-1); /* nof lgamma(ess) to be substracted */

  {
    int i;

    for(i=0;i<nof_freq2s;++i){
      int freq2 = freq2mem[i];
      if(freq2) res += freq2 * lgamma(ess + i);
    }

    for(i=0;i<nof_freq2s;++i){
      int freq2 = freq2mem2[i];
      if(freq2) res -= freq2 * lgamma(ess + i);
    }
  }

  return res;
}


scorefun init_BDq_scorer(char* arg){

  ess = atof(arg);
  data_not_big = N<BIG_BDE_DATA; 
  nof_freq2s = MIN(N+1,BIG_BDE_DATA);

  freq2mem  = malloc(nof_freq2s * sizeof(int));
  freq2mem2 = malloc(nof_freq2s * sizeof(int));

  return bdq_score;
}

void free_BDq_scorer(){
  if(freq2mem)  free(freq2mem);
  if(freq2mem2) free(freq2mem2);
}
