#include <emmintrin.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _ZINM_HEADER
#define _ZINM_HEADER

// Control parameters.
#define HISTO_INIT_SIZE 128
#define ZINM_MAXITER 32
#define ZINM_TOL 1e-6


struct zinb_part_t;

typedef struct zinb_par_t zinb_par_t;

struct zinb_par_t {
   double   a;
   double   p;
   double   pi;
};

// Compute the maximum likelihood estimates for NB and
// ZINB distributions, and return the paramters.
zinb_par_t * mle_nb   (int *, size_t);
zinb_par_t * mle_zinb (int *, size_t);
#endif
