#include <math.h>
#include <stdlib.h>
#include "get_local_scores.h"
#include "ilogi.h"
#include "reg.h"
#include "xtab.h"
#include "ls_qNML.h"
#include "stdio.h"

extern xtab**  ctbs; /* a working memory to collect  frequences */

score_t* qnml_scoretable = 0; /* nml probabilities for data column subsets */
scorefun qnml_vs_scorer;

#define BIG_NML_DATA (1<<16)

/* NML = log(ML) - log(REGRET) */

score_t big_qnml_score_(int i, varset_t vs, int len_vs){
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
	res -= logreg(N, get_nof_cfgs(vs)); /* regret */
	return res;
}

score_t qnml_score_(int i, varset_t vs, int len_vs){
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
	res -= logreg(N, (int)(get_nof_cfgs(vs))); /* regret */
	printf("%d %d %d %f %f\n", i, vs, len_vs, get_nof_cfgs(vs),res);
	return res;
}

score_t qnml_score(int i, varset_t psi, int dummy){
	return qnml_scoretable[psi | (1U<<i)] - qnml_scoretable[psi];
}

scorefun init_qNML_scorer(const char* logregfile){
  init_logreg(logregfile, nof_vars, nof_vals);
  qnml_scoretable = (score_t*) malloc((1<<nof_vars)*sizeof(score_t));
  if (N<BIG_NML_DATA){
    ensure_ilogi(N);
    qnml_vs_scorer =  qnml_score_;
  } else {
    ensure_ilogi(BIG_NML_DATA);
    qnml_vs_scorer =  big_qnml_score_;
  }
  return qnml_score;
}

void free_qNML_scorer(){
  free(qnml_scoretable);
  free_ilogi();
  free_logreg(nof_vars, nof_vals);
}
