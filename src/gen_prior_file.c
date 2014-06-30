#include "cfg.h"

#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "gopt.h"

#include "files.h"

/******** G L O B A L S *********/

/* Data format */

int  nof_vars;

/* Prior */

FILE* priorf = NULL;

/* Result file and output buffer */

#define BUFFER_SIZE (1048576)
score_t* buffer;
score_t* buffer_end;
score_t* buffer_ptr;

score_t* logprior;

/***** Initialising globals *****/

void create_log_prior()
{
  int n; int k;
  score_t* t = logprior = (score_t*) calloc(nof_vars, sizeof(score_t));
  for (n=1;n<nof_vars;++n) {
    score_t old = t[0] = 1.0;
    for (k=1; k<n+1; ++k){
      score_t new = old + t[k];
      old = t[k];
      t[k] = new;
    }
  }  
  for(k=1;k<nof_vars;++k) t[k] += t[k-1];
  for(k=0;k<nof_vars;++k) t[k] = -log(t[k]);
}

void create_output_buffer()
{
  buffer = (score_t*) calloc(BUFFER_SIZE, sizeof(score_t));
  buffer_end = buffer + BUFFER_SIZE;
  buffer_ptr = buffer;
}

void flush_n_free_output_buffer()
{
  if (buffer_ptr != buffer){
    fwrite(buffer, sizeof(score_t), buffer_ptr-buffer, priorf);
    buffer_ptr = buffer;
  }
  free(buffer);
}

void init_globals(const char* priorfile) {
  create_log_prior();
  priorf = (strcmp("-", priorfile) == 0) ? stdout : fopen(priorfile, "wb");
  create_output_buffer();
}

void free_globals(){
  flush_n_free_output_buffer();
  free(logprior);
  if(priorf != stdout) fclose(priorf);

}

/********* AND THE MAIN STUFF ******** */

void priors(int len_vs, varset_t vs)
{
  int i;
  for(i=0; i<nof_vars; ++i){
    varset_t iset = 1U<<i;
    if (vs & iset) {
      vs ^= iset;
      *buffer_ptr++ = logprior[len_vs-1];
      if (buffer_ptr == buffer_end){
	fwrite(buffer, sizeof(score_t), BUFFER_SIZE, priorf);
	buffer_ptr = buffer;
      }
      vs ^= iset;
    }
  }
}

void walk_contabs(int len_vs, varset_t vs, int nof_calls)
{

  priors(len_vs, vs);

  if (len_vs > 1){
    int i;
    for (i=0; i<nof_calls; ++i) {
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
      gopt_option('n', GOPT_ARG, gopt_shorts(0), gopt_longs( "nof-tasks" )),
      gopt_option('i', GOPT_ARG, gopt_shorts(0), gopt_longs( "task-index" ))
						    )
			    );
  if (argc != 3){
    fprintf(stderr, 
	    "Usage: get_prior_file nof_vars priorfile\n"
	    " --nof-tasks n\n"
            " --task-index i\n"
	    );
    return 1;
  } else {
    const char* nof_tasks_s;
    const char* task_index_s;
    int nof_tasks   = 1;
    int task_index  = 0;
    int nof_fixvars = 0;
    const char* priorfile = NULL;
    int len_vs;
    varset_t vs;
    
    if (gopt_arg(options, 'n', &nof_tasks_s))  nof_tasks  = atoi(nof_tasks_s);
    if (gopt_arg(options, 'i', &task_index_s)) task_index = atoi(task_index_s);
    
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
    
    nof_vars  = atoi(argv[1]);
    priorfile = argv[2];
    
    init_globals(priorfile);
    vs = task_index2varset(nof_vars - nof_fixvars, task_index);

    {
      int i;

      /* Sync ctbs to vs 
	 - maybe you could have just gathered these in the first place */

      len_vs = nof_vars; 
      for(i=0; i<nof_vars; ++i)
	if(!(vs & (1U<<i)))
	  len_vs--;
    }

    walk_contabs(len_vs, vs, nof_vars-nof_fixvars);

    free_globals();
  }
       
  gopt_free(options);

  return 0;
}
