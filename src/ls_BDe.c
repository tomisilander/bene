#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "get_local_scores.h"
#include "ilogi.h"
#include "ls_BDe.h"

extern int*     nof_vals;
extern int      N;

score_t ess;

int*    freq2mem  = NULL;
int*    freq2mem2 = NULL;
int     nof_freq2s;

static int is_BDeu = 1;
static int is_Cow  = 0;

#define BIG_BDE_DATA (1<<10)

static int data_not_big = 0;

extern double lgamma(double); /* to avoid the warning with ansi-flag */

/* BDe */

score_t bde_score(int i, varset_t psi, int nof_freqs){

  int vc_v = nof_vals[i];
  score_t pcc =  get_nof_cfgs(psi);
  score_t ess_per_cc  = is_BDeu ? ess/(pcc*vc_v) : ess;
  score_t ess_per_pcc = ess_per_cc * vc_v;
  score_t xpenalty = is_Cow ? log(pcc*vc_v) : 0.0;

  int* freqp = freqmem;
  int* end_freqp = freqp + nof_freqs * vc_v;

  score_t res = nof_freqs * lgamma(ess_per_pcc);
  int nof_zeros = 0;
    
  memset(freq2mem,  0, nof_freq2s*sizeof(int)); /* could reset later */
  memset(freq2mem2, 0, nof_freq2s*sizeof(int)); /* could reset later */

  for(;freqp < end_freqp; freqp += vc_v) {
    int  pcfreq = 0;
    int v;
    for(v = 0; v<vc_v; ++v) {
      int freq = freqp[v];
      if (freq) {
	pcfreq += freq;
	if(data_not_big || freq<nof_freq2s){
	  ++freq2mem[freq];
	} else {
	  res += lgamma(ess_per_cc + freq);
	}
      } else {
	++ nof_zeros;
      }
    }
    if(data_not_big || pcfreq<nof_freq2s){
      ++freq2mem2[pcfreq];
    } else {
      res -= lgamma(ess_per_pcc + pcfreq);
    }
  }

  res += (nof_zeros - nof_freqs * vc_v) * lgamma(ess_per_cc);

  {
    int i;

    for(i=1;i<nof_freq2s;++i){
      int freq2 = freq2mem[i];
      if(freq2) res += freq2 * lgamma(ess_per_cc + i);
    }

    for(i=1;i<nof_freq2s;++i){
      int freq2 = freq2mem2[i];
      if(freq2) res -= freq2 * lgamma(ess_per_pcc + i);
    }

  }

  return res > 0.0 ? 0.0 : res - xpenalty;
}


scorefun init_BDe_scorer(char* arg){

  char scoretype = arg[strlen(arg)-1];

  if (scoretype == 'R' || scoretype == 'r') {
    is_BDeu = 0;
  } else if (scoretype == 'c' || scoretype == 'C') {
    is_Cow = 1;
  }

  ess = atof(arg);
  data_not_big = N<BIG_BDE_DATA; 
  if (data_not_big){
    nof_freq2s = N+1;
  } else {
    nof_freq2s = BIG_BDE_DATA;
  }

  freq2mem  = malloc(nof_freq2s * sizeof(int));
  freq2mem2 = malloc(nof_freq2s * sizeof(int));

  return bde_score;
}

void free_BDe_scorer(){
  if(freq2mem)  free(freq2mem);
  if(freq2mem2) free(freq2mem2);
}
