#include <math.h>
#include <stdlib.h>
#include "get_local_scores.h"
#include "ilogi.h"
#include "reg.h"
#include "ls_qNML.h"

extern int*     nof_vals;
extern int      N;

double *logregNq = NULL;
int    size_logregNq = 0;

#define BIG_NML_DATA (1<<16)

/* qNML = log(ML(X,Pa(X)) - log(REGRET(X,Pa(X))
        - log(ML(Pa(X))   + log(REGRET(Pa(X))

        = log(ML(X|Pa(X)) + log(REGRET(Pa(X)) - log(REGRET(X,Pa(X)) 
*/

extern double lgamma(double); /* to avoid the warning with ansi-flag */

double log_nCk(int n, int k) {
  return lgamma(n+1) - lgamma(k+1) - lgamma(n-k+1);
}

double reg2(int n){
  double logN = log(n);
  double res = 2.0;
  int k;
  for (k=1;k<n;++k){
    res += exp(log_nCk(N, k) + k * (log(k) - logN) + (N - k) * (log(N - k) - logN));
  }
  return res;
}


void init_logregNq(int nof_regs){
  size_logregNq = nof_regs;
  logregNq = malloc(nof_regs*sizeof(double));
  logregNq[0] = 0.0; /* never used */
  logregNq[1] = 0.0;
  logregNq[2] = log(reg2(N));
  {
    double a = logregNq[1];
    double b = logregNq[2];
    double logN = log(N);
    int k;
    for (k=3; k<nof_regs; ++k){
      double q = logN - log(k - 2) + a;
      double x0, x1;
      if (q<b){
	x0 = q; x1 = b;
      } else {
	x0 = b; x1 = q;
      }
      logregNq[k] = x1 + log1p(exp(x0 - x1));
      a = b; b = logregNq[k];
    }
  }
}	  

double approx_szp2(int N, double r) {
  double a = r/N;
  double Ca = 0.5*(1+sqrt(1+4/a));
  return N*(log(a)+(a+2)*log(Ca)-1/Ca)-0.5*log(Ca+2/a);
}

score_t qregdiff(double K, int r) {
  if (r*K < size_logregNq) {
    return logregNq[(int)K] - logregNq[r*(int)K];
  } else {
    return approx_szp2(N,K)-approx_szp2(N,K*r);
  }
}

score_t qnml_score(int i, varset_t psi, int nof_freqs){

  int vc_v = nof_vals[i];
  double pcc =  get_nof_cfgs(psi);
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
  res += qregdiff(pcc,vc_v);

  return res;
}

scorefun init_qNML_scorer(const char* logregfile){
  init_logregNq(100);
  ensure_ilogi(MIN(N,BIG_NML_DATA));
  return qnml_score;
}

void free_qNML_scorer(){
  free_ilogi();
  free(logregNq);
  logregNq = NULL;
}
