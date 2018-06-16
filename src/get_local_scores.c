#include "cfg.h"

#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "gopt.h"

#include "xtab.h"
#include "reg.h"
#include "files.h"

#include "ls_XIC.h"
#include "ls_fNML.h"
#include "ls_qNML.h"
#include "ls_BDe.h"
#include "ls_BDq.h"
#include "ls_LOO.h"
#include "get_local_scores.h"


/******** G L O B A L S *********/

/* Data format */

int  N;
int  nof_vars;
int  nof_cols;
int* nof_vals;
int* sel_vars;

/* Hashing and working memory */

#define RANGE  (1024)
uint** xh;  /* a random number [0..RANGE[ for each value of each variable  */
uchar** keymem;  /* for each number of vars emory for configs and counts   */ 
xtab**  ctbs;    /* for each number of vars hash table for configs         */ 

int*    freqmem; /* a working memory to collect the conditional frequences */

/* Constraints */

varset_t* musts = 0;
varset_t* nopes = 0;

/* Scoring */

scorefun scorer;
scorefree free_scorer;

/* Prior */

FILE* priorf = NULL;
#define LOG2 (0.69314718055994530941)
int use_MU = 0;

/* maximum number of parents */

int max_parents = 1024;

/* Result file and output buffer */

FILE* resultf;
#define BUFFER_SIZE (1048576)
score_t* buffer;
score_t* buffer_end;
score_t* buffer_ptr;


/***** Initialising globals *****/

void init_data_format(const char* vdfile, const char* datfile, const char* selfile) {
  nof_cols = nof_lines(vdfile);
  nof_vars = (selfile == NULL) ? nof_cols : nof_lines(selfile);
  N        = nof_lines(datfile);
  nof_vals = (int*) calloc(nof_vars, sizeof(int));    
  sel_vars = (int*) calloc(nof_vars, sizeof(int));

  /* List selected variables */
  if (selfile == NULL){
    int i; for (i=0; i<nof_vars; ++i) sel_vars[i]=i;
  } else {
    FILE* self;
    int i = 0;

    if ((self = fopen(selfile,"r"))==NULL){
      fprintf(stderr,"Cannot open selectfile %s\n",selfile);
      exit(1);
    }

    for (i=0; i<nof_vars; ++i) 
      if (1 != fscanf(self, "%d", sel_vars+i)) {
	fprintf(stderr, "Reading selected variable number %d failed\n", i);
	exit(1);
      }
    fclose(self);
  }

  { /* Count values of selected variables */
    FILE* vdf = fopen(vdfile,"r");
    int i = 0; /* line number in vdfile */
    int j = 0; /* index of selected variable */
    int c = 0;
    int prev = 'X';
    while((EOF != (c=fgetc(vdf))) && (j<nof_vars)){
      if ((c=='\r') || (c=='\n' && prev!='\r')) {
	/* line ended (but \n after \r may remain) */
	j += (sel_vars[j] == i++); 
      } else if ((sel_vars[j] == i) && (c != '\n') ){
      	nof_vals[j] += (c=='\t');
      }
      prev = c;
    }
    fclose(vdf);
  }
}

void init_xh(){
  int i;
  srand(666); 
  xh = calloc(nof_vars, sizeof(int*));
  for(i=0; i<nof_vars; ++i){
    int v;
    xh[i] = calloc(nof_vals[i], sizeof(uint));
    for(v=0; v<nof_vals[i]; ++v){
      xh[i][v] = (int) ((score_t)RANGE*rand()/(RAND_MAX+1.0));
    }
  }
}

xtab* dat2ctb(const char* datfile) {
  xtab* ctb = xcreate(RANGE, N);
  FILE* datf = fopen(datfile,"r");
  uchar index[MAX_NOF_VARS];
  int i  = 0; /* column index */
  int j  = 0; /* variable index */
  uint h = 0;
  int v  = 0;

  while(1 == fscanf(datf, "%d", &v)){
    i %= nof_cols;
    if (i++ != sel_vars[j]) continue;

    h ^= xh[j][v];
    index[j++] = v;

    if (j == nof_vars){
      int new = 0;
      xentry* x = xadd(ctb, h, index, nof_vars, &new);
      if(new) {
	x->key = memcpy(malloc(nof_vars), index, nof_vars);
	x->val = calloc(1, sizeof(int));
      } 
      ++ *x->val;
      h = j = 0;
    }
  }
  fclose(datf);
  return ctb;
}

void datctb2ctb(xtab* ctb) {
  xtab* ctb2    = ctbs[nof_vars];
  uchar* keymp2 = keymem[nof_vars];
  xentry* x;

  for(x = ctb->xentries; x<ctb->free; ++x){
    int new = 1;
    xentry* x2 = xadd(ctb2, x->h, x->key, nof_vars, &new);
    x2->key = memcpy(keymp2, x->key, nof_vars);
    keymp2 += nof_vars;
    x2->val = memcpy(keymp2, x->val, sizeof(int));
    keymp2 += sizeof(int);
  }

}

/* keymem is a misnomer. It has space for keys and the keycounts. */

void get_keymem(int nof_keys){
  int i;
  keymem  = (uchar**) malloc((nof_vars+1)*sizeof(uchar*));
  for(i=0;i<nof_vars+1;++i){
    keymem[i] = (uchar*) malloc(nof_keys * (nof_vars + sizeof(int)));
  }
}


void get_freqmem(int nof_keys){
  int max_nof_vals = 0;
  int i;
  for(i=0; i<nof_vars; ++i)
    if(nof_vals[i] > max_nof_vals)
      max_nof_vals = nof_vals[i];
  
  freqmem = (int*) malloc(nof_keys * max_nof_vals * sizeof(int));

}

void get_ctbs(int nof_keys){
  int i;

  ctbs  = calloc(nof_vars+1, sizeof(xtab*));
  for(i=0; i<nof_vars+1; ++i) 
      ctbs[i] = xcreate(RANGE, nof_keys);
}

void init_memory(const char* datfile, char* essarg){
  xtab* ctb0       = dat2ctb(datfile);
  int max_nof_keys = xcount(ctb0);

  get_keymem(max_nof_keys);
  get_ctbs(max_nof_keys);
  datctb2ctb(ctb0);

  { 
    xentry* x;
    for(x = ctb0->xentries; x<ctb0->free; ++x){
      free(x->key);
      free(x->val);
    }
    xdestroy(ctb0);
  }

  get_freqmem(max_nof_keys);

}    

void get_constraints(const char* filename) 
{
  FILE* constrf = fopen(filename,"r");
  char yn[20];
  int src, dst;

  musts = (varset_t*) calloc(nof_vars,sizeof(varset_t));
  nopes = (varset_t*) calloc(nof_vars,sizeof(varset_t));

  while(3 == fscanf(constrf, "%s %d %d", yn, &src, &dst)){
    if (yn[0]=='+') {
      musts[dst] |= 1<<src;
      nopes[dst] &= ~(1<<src);
    } else if (yn[0]=='-') {
      nopes[dst] |= 1<<src;
      musts[dst] &= ~(1<<src);
    } else {
      fprintf(stderr,"Unknown constraint type %s\n",yn);
      exit(4);
    }
  }
  fclose(constrf);
}

void free_constraints()
{
  free(musts);
  free(nopes);
}

void create_output_buffer(const char* priorfile)
{
  buffer = (score_t*) calloc(BUFFER_SIZE, sizeof(score_t));
  buffer_end = buffer + BUFFER_SIZE;
  buffer_ptr = buffer;
  if(priorfile != NULL) {
    if (strcmp(priorfile,"@MU") == 0){
        use_MU = 1;
    } else {
      int _nof_priors;
      priorf = fopen(priorfile, "rb");
      _nof_priors = fread(buffer, sizeof(score_t), BUFFER_SIZE, priorf);
      _nof_priors ++; /* to fget rid of compiler warning  */
    }
  }
}

void flush_n_free_output_buffer()
{
  if (buffer_ptr != buffer){
    fwrite(buffer, sizeof(score_t), buffer_ptr-buffer, resultf);
    buffer_ptr = buffer;
  }
  free(buffer);
}

score_t get_nof_cfgs(varset_t vs)
{
  score_t res = 1;
  int i;
  for(i=0; i<nof_vars; ++i) {
    if(vs & (1U<<i)) {
      res *= nof_vals[i];
    }
  }
  return res;
}

void init_scorer(char* essarg, const char* logregfile) {
  int arglen = strlen(essarg); /* how about checking valid length */
  
  if((0==strncmp(essarg, "BIC", 3)) || (0==strncmp(essarg, "AIC", 3)) ) {
    scorer = init_XIC_scorer(essarg);
    free_scorer = free_XIC_scorer;
  } else if(0==strncmp(essarg, "fNML", 4)) {
    scorer = init_fNML_scorer(logregfile);
    free_scorer = free_fNML_scorer;
  } else if(0==strncmp(essarg, "qNML", 4)) {
    scorer = init_qNML_scorer(logregfile);
    free_scorer = free_qNML_scorer;
  } else if((essarg[arglen-1]=='L') || (essarg[arglen-1]=='l')) {
    scorer = init_LOO_scorer(essarg);
    free_scorer = free_LOO_scorer;
  } else if((essarg[arglen-1]=='Q') || (essarg[arglen-1]=='q')) {
    scorer = init_BDq_scorer(essarg);
    free_scorer = free_BDq_scorer;
  } else { /* more checks please */
    scorer = init_BDe_scorer(essarg);
    free_scorer = free_BDe_scorer;
  }
}

void init_globals(const char* vdfile, const char* datfile, char* essarg, const char* resfile,
		  const char* cstrfile, const char* priorfile, 
                  const char* logregfile, const char* selfile) {
  resultf = (strcmp("-", resfile) == 0) ? stdout : fopen(resfile, "wb");
  create_output_buffer(priorfile);
  init_data_format(vdfile, datfile, selfile);
  init_xh();
  init_memory(datfile, essarg);
  init_scorer(essarg, logregfile);
  if(cstrfile) get_constraints(cstrfile);
}

void free_globals(){
  int i;
  
  free(freqmem);

  for(i=0; i<nof_vars+1; ++i) xdestroy(ctbs[i]);
  free(ctbs);

  for(i=0; i<nof_vars+1; ++i) free(keymem[i]);
  free(keymem);

  for(i=0; i<nof_vars; ++i) free(xh[i]);
  free(xh);

  free_scorer();

  if (musts != 0) free_constraints();

  flush_n_free_output_buffer();
  if(resultf != stdout) fclose(resultf);
  if(priorf != NULL) fclose(priorf);

  free(nof_vals);
  free(sel_vars);

}

/********* AND THE MAIN STUFF ******** */

/****** Gathering counts ******/

void contab2contab(int i, int len_vs)
{
  xtab* ctb  = ctbs[len_vs];
  xtab* ctb2 = ctbs[len_vs - 1];

  uchar* keymp2 = keymem[len_vs-1];

  int len_ctb = xcount(ctb);
  uint* xhi = xh[i];
  int k;
  
  xreset(ctb2);

  /* go through all the keys of the ctb 
     and build the corresponding keys and freqs to ctb2 */

  for(k=0; k<len_ctb; ++k) {
    xentry* xk = ctb->xentries + k;
    xentry* x2;
    uchar* key   = xk->key;
    uint h = xk->h ^ xhi[key[i]];
    int new = 0;
    uchar tmp = key[i];
    key[i] = 0;

    x2 = xadd(ctb2, h, key, nof_vars, &new);

    if(new) {
      x2->key = memcpy(keymp2, key, nof_vars);
      keymp2 += nof_vars;

      *(x2->val = (int*) keymp2) = 0;
      keymp2 += sizeof(int);
    }

    *x2->val += *xk->val;

    key[i] = tmp;
  }
}

int contab2condtab(int i, int len_vs)
{
  int vc_i   = nof_vals[i];
  int* freqp = freqmem;

  xtab* ctb  = ctbs[len_vs];
  xtab* ctb2 = ctbs[0];

  uchar* keymp2    = keymem[0];
  int k;
  int len_ctb = xcount(ctb);
  uint* xhi = xh[i];

  xreset(ctb2);

  memset(freqp, 0, len_ctb*vc_i*sizeof(int));

  for(k=0; k<len_ctb; ++k) {
    xentry* xk = ctb->xentries + k;
    xentry* x;
    uchar* key   = xk->key;
    uchar val_i = key[i];
    uint h = xk->h ^ xhi[val_i];
    int new = 0;
    key[i] = 'X';

    x = xadd(ctb2, h, key, nof_vars, &new);

    if(new) {
      x->key = memcpy(keymp2, key, nof_vars);
      keymp2 += nof_vars;

      x->val = freqp;
      freqp  += vc_i;
    }

    x->val[val_i] += *xk->val;

    key[i] = val_i;

  }

  return xcount(ctb2);
}

void scores(int len_vs, varset_t vs)
{
  int i;
  int nof_parents = len_vs - 1;
  for(i=0; i<nof_vars; ++i){
    varset_t iset = 1U<<i;
    if (vs & iset) {
      int nof_freqs = contab2condtab(i, len_vs);
      vs ^= iset;
      if (nof_parents > max_parents) {
	*buffer_ptr++ = (score_t) MIN_NODE_SCORE;
      } else if (musts && 
	  (((musts[i] & vs) != musts[i]) || ((nopes[i] & vs) != 0))) { 
	*buffer_ptr++ = (score_t) MIN_NODE_SCORE;
      } else {
	*buffer_ptr++ += scorer(i, vs, nof_freqs) - use_MU*LOG2*nof_parents;
      }
      if (buffer_ptr == buffer_end){
	fwrite(buffer, sizeof(score_t), BUFFER_SIZE, resultf);
	if (priorf == NULL){
	  memset(buffer, 0, BUFFER_SIZE*sizeof(score_t));
	} else {
	  int _nof_priors = fread(buffer, sizeof(score_t), BUFFER_SIZE, priorf);
         _nof_priors ++; /* to fget rid of compiler warning about unused variable */
	}
	buffer_ptr = buffer;
      }
      vs ^= iset;
    }
  }
}

#if 0
void walk_contabs0(int len_vs, varset_t vs, int nof_calls)
{
	qnml_scoretable[vs] = qnml_vs_scorer(0,vs,len_vs);

	if (len_vs > 1){
	int i;
	for (i=0; i<nof_calls; ++i) {
	contab2contab(i, len_vs);
	walk_contabs0(len_vs-1, vs^(1U<<i), i);
    }
  }
}
#endif

void walk_contabs(int len_vs, varset_t vs, int nof_calls)
{

  scores(len_vs, vs);

  if (len_vs > 1){
    int i;
    for (i=0; i<nof_calls; ++i) {
      contab2contab(i, len_vs);
      walk_contabs(len_vs-1, vs^(1U<<i), i);
    }
  }
}

int ilog2(int i) /* i has to be > 0 */
{
  int n = 0;
  while(i>>=1) ++n;
  return n;
}
    

varset_t task_index2varset(int nof_taskvars, int task_index)
{
  varset_t allvars = LARGEST_SET(nof_vars);
  varset_t fixvars = task_index << nof_taskvars;
  return (~fixvars) & allvars;
}
 
int main(int argc, char* argv[])
{

  void *options= gopt_sort( &argc, (const char**)argv, gopt_start(
      gopt_option('l', GOPT_ARG, gopt_shorts('l'), gopt_longs( "logregfile" )),
      gopt_option('n', GOPT_ARG, gopt_shorts(0),   gopt_longs( "nof-tasks" )),
      gopt_option('i', GOPT_ARG, gopt_shorts(0),   gopt_longs( "task-index" )),
      gopt_option('c', GOPT_ARG, gopt_shorts('c'), gopt_longs( "constraints" )),
      gopt_option('p', GOPT_ARG, gopt_shorts('p'), gopt_longs( "prior" )),
      gopt_option('s', GOPT_ARG, gopt_shorts('s'), gopt_longs( "selectvars" )),
      gopt_option('m', GOPT_ARG, gopt_shorts('m'), gopt_longs( "max-parents" ))
						    )
			    );
  if (argc != 5){
    fprintf(stderr, 
	    "Usage: get_local_scores vdfile datfile "
	    "(ess[l|L|r|R|q|Q|S|s] | AIC | BIC | fNML | qNML) resfile \n" 
            " -l --logregfile file : needed for fNML and qNML\n"
	    " --nof-tasks n\n"
            " --task-index i\n"
            " -c --constraints cstrfile\n"
            " -p --prior priorfile\n"
	    " -m --max-parents\n"
	    " -s --selectvars selfile\n"
);
    return 1;
  } else {
    const char* nof_tasks_s;
    const char* task_index_s;
    const char* max_parents_s;
    int nof_tasks   = 1;
    int task_index  = 0;
    int nof_fixvars = 0;
    int len_vs;
    const char* cstrfile   = NULL;
    const char* priorfile  = NULL;
    const char* logregfile = NULL;
    const char* selfile = NULL;
    varset_t vs;
    
    if (gopt_arg(options, 'n', &nof_tasks_s))  nof_tasks  = atoi(nof_tasks_s);
    if (gopt_arg(options, 'i', &task_index_s)) task_index = atoi(task_index_s);
    if (gopt_arg(options, 'c', &cstrfile)){} ; /* could check file etc. */
    if (gopt_arg(options, 'p', &priorfile)){} ; /* could check file etc. */
    if (gopt_arg(options, 'l', &logregfile)){} ; /* could check file etc. */
    if (gopt_arg(options, 's', &selfile)){} ; /* could check file */
    if (gopt_arg(options, 'm', &max_parents_s)) max_parents = atoi(max_parents_s);
    
    if (task_index >= nof_tasks) {
      fprintf(stderr, 
	      "task index must be less than nof_tasks\n");
      return 2;
    }

    nof_fixvars = ilog2(nof_tasks);
    if(nof_tasks != (1<<nof_fixvars)) {
      fprintf(stderr, 
	      "nof_tasks has to be a power of two - %d is not\n", nof_tasks);
      return 3;
    }
    
    init_globals(argv[1], argv[2], argv[3], argv[argc-1], cstrfile, priorfile, logregfile, selfile);
    vs = task_index2varset(nof_vars - nof_fixvars, task_index);

    {
      int i;

      /* Sync ctbs to vs 
	 - maybe you could have just gathered these in the first place */

      len_vs = nof_vars; 
      for(i=0; i<nof_vars; ++i)
	if(!(vs & (1U<<i)))
	  contab2contab(i, len_vs--);
    }

#if 0
    if(qnml_scoretable != 0) walk_contabs0(len_vs, vs, nof_vars-nof_fixvars);
#endif

    walk_contabs(len_vs, vs, nof_vars-nof_fixvars);

    free_globals();
  }
       
  gopt_free(options);

  return 0;
}
