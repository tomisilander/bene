#include <math.h>
#include <stdlib.h>
#include "get_local_scores.h"
#include "ilogi.h"
#include "reg.h"
#include "xtab.h"
#include "ls_cNML.h"

extern xtab**  ctbs; /* a working memory to collect  frequences */

score_t* cnml_scoretable = 0; /* nml probabilities for data column subsets */
scorefun cnml_vs_scorer;

#define BIG_NML_DATA (1<<16)

/* NML = log(ML) - log(REGRET) */

score_t big_cnml_score_(int i, varset_t vs, int len_vs){
	xtab* ctb  = ctbs[len_vs];
	int len_ctb = xcount(ctb);
	xentry* xp = ctb->xentries;
	xentry* end_xp = xp + len_ctb;

	score_t res = 0.0;
	for(; xp<end_xp; xp++) {
		int freq = *xp->val;
		res += (freq < BIG_NML_DATA) ? ilogi[freq] : freq * log(freq);
	}
	res -= (N < BIG_NML_DATA) ? ilogi[N] : N * log(N);
	res -= log(reg(N, get_nof_cfgs(vs))); /* regret */
	return res;
}

score_t cnml_score_(int i, varset_t vs, int len_vs){
	xtab* ctb  = ctbs[len_vs];
	int len_ctb = xcount(ctb);
	xentry* xp = ctb->xentries;
	xentry* end_xp = xp + len_ctb;

	score_t res = 0.0;
	for(; xp<end_xp; xp++) {
		int freq = *xp->val;
		res += ilogi[freq] ;
	}
	res -= ilogi[N];
	res -= log(reg(N, get_nof_cfgs(vs))); /* regret */
	return res;
}

score_t cnml_score(int i, varset_t psi, int dummy){
	return cnml_scoretable[psi | (1U<<i)] - cnml_scoretable[psi];
}

scorefun init_cNML_scorer(){
  cnml_scoretable = (score_t*) malloc((1<<nof_vars)*sizeof(score_t));
  if (N<BIG_NML_DATA){
    ensure_ilogi(N);
    cnml_vs_scorer =  cnml_score_;
  } else {
    ensure_ilogi(BIG_NML_DATA);
    cnml_vs_scorer =  big_cnml_score_;
  }
  return cnml_score;
}

void free_cNML_scorer(){
  free(cnml_scoretable);
  free_ilogi();
}
