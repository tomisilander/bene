#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "files.h"
#include "reg.h"

#define pi 3.1415926535897932384626433832795

static float hlogpi = 0.0;
static float r1_s2  = 0.0;
static float** logregtab = NULL;

void init_globconsts() {
  hlogpi = 5*log(pi);
  r1_s2  = 1/sqrt(2);
}

void init_logreg(const char* logregfile, int nof_vars, int* nof_vals){
  
  if (hlogpi == 0.0) init_globconsts();

  if (! logregtab) logregtab = (float**) calloc(256, sizeof(float*));
  {
    int i;
    FILE* logregf = NULL;

    for(i=0; i<nof_vars; ++i){
      int vci = nof_vals[i];
      if (! logregtab[vci-1]) {
	logregtab[vci-1] = (float*) malloc(2001 * sizeof(float));
	if(!logregf) logregf = fopen(logregfile, "rb");
	fseek(logregf,(vci-1)*2001*sizeof(float),SEEK_SET);
	FREAD(logregtab[vci-1], sizeof(float), 2001, logregf);
      }
    }
    if (logregf) fclose(logregf);
  }
}

/* nobody uses this ? */

void init_logregN(const char* logregfile){
  
  init_globconsts();

  logregtab = (float**) calloc(256, sizeof(float*));
  {
    int vci;
    FILE* logregf = fopen(logregfile, "rb");

    for (vci=1;vci<=256;++vci) { 
      if (logregtab[vci-1]==NULL) {
	logregtab[vci-1] = (float*) malloc(2001 * sizeof(float));
	FREAD(logregtab[vci-1], sizeof(float), 2001, logregf);
      }
    }
    fclose(logregf);
  }
}


void free_logreg(int nof_vars, int* nof_vals){
  int i;
  for(i=0; i<nof_vars; ++i){
    int vci = nof_vals[i];
    if (logregtab[vci-1]) {
      free(logregtab[vci-1]);
      logregtab[vci-1]=NULL;
    }
  }
  free(logregtab);
  logregtab = NULL;
}

float logreg(int N, int K){
  
  if (K<=256 && N<=2000){
    return logregtab[K-1][N];
  } else {
    score_t hK   = 0.5*K;
    score_t hK_h = hK-0.5;
    return (hK_h) * log(N/2.0)
      + hlogpi - lgamma(hK)			  
      + 0.5 - pow(r1_s2 - (K/3/sqrt(N)) * exp(lgamma(hK)-lgamma(hK_h)), 2)
      + (3+K*(K-2)*(2*K +1))/(36.0*N);
  }
}

#ifdef MAIN
#include <stdio.h>
#include <stdlib.h>

static score_t reg(int N, int K){
  return exp(logreg(N,K));
}

int main(int argc, char* argv[]){
  int N, K;

  if (argc != 4) {
    fprintf(stderr, "Usage: reg N K logregfile\n");
    exit(2);
  }

  N = atoi(argv[1]);
  K = atoi(argv[2]);

  if (K<=0){
    fprintf(stderr, "K has to be a positive integer.\n");
    exit(2);
  }
     
  if (N<0){
    fprintf(stderr, "N has to be a non-negative integer.\n");
    exit(2);
  }


  if (K<=256 && N<=2000) init_logreg(argv[3], 1, &K);
  printf("%f\n", logreg(N,K));
  if (K<=256 && N<=2000) free_logreg(1, &K);

  return 0;
}
	
#endif
