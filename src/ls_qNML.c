#include <math.h>
#include "get_local_scores.h"
#include "ilogi.h"
#include "reg.h"
#include "ls_qNML.h"

extern int*     nof_vals;
extern int      N;

#define BIG_NML_DATA (1<<16)

/* qNML = log(ML(X,Pa(X)) - log(REGRET(X,Pa(X))
        - log(ML(Pa(X))   + log(REGRET(Pa(X))

        = log(ML(X|Pa(X)) + log(REGRET(Pa(X)) - log(REGRET(X,Pa(X)) 
*/

/* Room for optimization, but later ... */

score_t qregret(score_t K, int r) {


  if (K < 1024) {
    return logreg(N,(int)K) - logreg(N,r*(int)K);
  } else {

    score_t log2_N_plus_1_9N = log(2.0/N) + 1/(9.0*N);
    score_t sqrt18N = sqrt(18*N);      

    /* precompute */
    
    score_t K_2 = K/2;
    score_t Kr  = K*r;
    score_t logr = log(r);

    
    score_t k =  (Kr/2-K_2)*log2_N_plus_1_9N;
    
    score_t apx3 = (5*logr + log((Kr+1)/(K+1)))/12;
    score_t tidyramad = (K_2)*((r-1)*log(K_2)+r*logr+1 - r);
    score_t klogk = tidyramad + apx3; 

    score_t k3_2 = ((1-Kr)*sqrt(2*Kr+1) + (K-1)*sqrt(2*K+1))/sqrt18N;
    
    return k - logr + k3_2 + klogk;
  }
}

score_t qnml_score(int i, varset_t psi, int nof_freqs){

  int vc_v = nof_vals[i];
  score_t pcc =  get_nof_cfgs(psi);
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
  }
  res += qregret(pcc,vc_v);

  return res;
}

scorefun init_qNML_scorer(const char* logregfile){
  init_logregN(logregfile);
  ensure_ilogi(MIN(N,BIG_NML_DATA));
  return qnml_score;
}

void free_qNML_scorer(){
  free_ilogi();
  free_logreg(nof_vars, nof_vals);
}
