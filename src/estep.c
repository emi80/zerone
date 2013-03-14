#include "estep.h"

void
spcfwdb (
   // input //
   const int n,
   const double *Q,
   const double *init,
   // output //
   double *pratio,
   double *phi,
   // not initialized //
   double *sumtrans
)
// SYNOPSIS:                                                             
//   Special forward-backward algorithm for 3-state chains where only    
//   the ratio of emission probabilities is given.                       
//                                                                       
// ARGUMENTS:                                                            
//   'n': length of the sequence of observations                         
//   'Q': (3,3) diagonal of the transition matrix                        
//   'init': (3) initial probability of the first state                  
//   'pratio': (2,n) ratio of emission probabilities                     
//   'phi': (2,n) smoothed probabilities for the first 2 states          
//   'sumtrans': (3,3) sum of conditional transitions probabilties       
//                                                                       
// ENCODING OF Q.                                                        
//   Q(i,j) is a transition from state i to state j, so Q[0] is the      
//   transition from state 0 to state 0, Q[1] is the transition from     
//   state 1 to state 0, Q[3] is the transition from state 0 to state    
//   1 etc. (the general term Q[i+3*j] is the transition from state i    
//   to state j).                                                        
//                                                                       
// RETURN:                                                               
//   'void'                                                              
//                                                                       
// SIDE EFFECTS:                                                         
//   Replace 'pratio' by alphas, update 'phi' and 'sumtrans' in place.   
{

   int j;
   int k;
   double sum;              // Computational intermediate.
   double tmp[3];           // Computational intermediate.
   double B[9];             // Diagonal of the reverse kernel.
   double *alpha = pratio;  // For clarity.

   // First iteration of the forward pass.
   sum = init[0]*pratio[0] + init[1]*pratio[1] + init[2];
   alpha[0] = init[0]*pratio[0] / sum;
   alpha[1] = init[1]*pratio[1] / sum;
   // a[2] = alpha[2] = init[2] / tmp;

   // Next iterations of the forward pass.
   for (k = 1 ; k < n ; k++) {
      for (j = 0 ; j < 3 ; j++) {
         tmp[j] = Q[  3*j] * alpha[2*k-2] + 
                  Q[1+3*j] * alpha[2*k-1] +
                  Q[2+3*j] * (1 - alpha[2*k-2] - alpha[2*k-1]);
      }
      sum = tmp[0]*pratio[2*k] + tmp[1]*pratio[2*k+1] + tmp[2];
      if (sum > 0) {
         alpha[2*k  ] = tmp[0]*pratio[2*k  ] / sum;
         alpha[2*k+1] = tmp[1]*pratio[2*k+1] / sum;
      }
      else {
         alpha[2*k+1] = alpha[2*k] = 1.0/3;
      }
   }

   // NB: 'pratio' now contains the forward alphas for the first state.
   // However, we refer to it as 'alpha' for clarity.
   // First iteration of the backward pass.
   phi[2*n-2] = alpha[2*n-2];
   phi[2*n-1] = alpha[2*n-1];

   // Next iterations of the backward pass.
   for (k = n-2 ; k >= 0 ; k--) {
      // The backward recursion kernel B(j,i) gives the probability of
      // a backward transition from state j to state i.
      for (j = 0 ; j < 3 ; j++) {

         sum = Q[  3*j] * alpha[2*k] +
               Q[1+3*j] * alpha[2*k+1] +
               Q[2+3*j] * (1 - alpha[2*k] - alpha[2*k+1]);

         if (sum > 0) {
            B[j  ] = Q[  3*j] * alpha[2*k] / sum;
            B[j+3] = Q[1+3*j] * alpha[2*k+1] / sum;
            B[j+6] = Q[2+3*j] * (1 - alpha[2*k] - alpha[2*k+1]) / sum;
         }
         else {
            B[j] = B[j+3] = B[j+6] = 1.0/3;
         }

         sumtrans[  3*j] = B[j  ] * phi[2*k+2];
         sumtrans[1+3*j] = B[j+3] * phi[2*k+3];
         sumtrans[2+3*j] = B[j+6] * (1 - phi[2*k+2] - phi[2*k+3]);

      }

      phi[2*k]   = B[0] * phi[2*k+2] +
                   B[1] * phi[2*k+3] +
                   B[2] * (1 - phi[2*k+2] - phi[2*k+3]);
      phi[2*k+1] = B[3] * phi[2*k+2] +
                   B[4] * phi[2*k+3] +
                   B[5] * (1 - phi[2*k+2] - phi[2*k+3]);

   }

   return;

}

void
compute_pratio (
   // input //
   int *n_obs,
   int *dim_z,
   int *y,
   int *z,
   // params //
   double *a,
   double *b,
   double *gamma,
   // output //
   double *pratio
)
// SYNOPSIS:                                                             
//   Compute emission probabilities with a negative multinomial model.   
//   Since those are up to a multiplicative constant in the forward-     
//   backward algorithm, we can compute the ratios of the emission       
//   probabilities relative to a reference state, of which we set the    
//   emission probability to 1. This saves computation for one state.    
//                                                                       
//      Since the negative nultinomial takes discrete values, the time   
//   series takes a limited number of values. This means that we can     
//   cache the results for reuse in order to save computation.           
//                                                                       
//      My parametrization is of the form                                
//                                                                       
//       p_0(i)^alpha * p_1(i)^y * p_2(i)^z_1 * ... * p_r+1(i)^z_r       
//                                                                       
//      By taking the ratio relative to state i=3 the terms to compute   
//   are of the form                                                     
//                                                                       
//       q_0(i)^alpha * q_1(i)^y * q_2(i)^z_1 * ... * q_r+1(i)^z_r       
//                                                                       
//   where q_0(i) = p_0(i)/p_0(3) etc.                                   
//                                                                       
// ARGUMENTS:                                                            
//   'n_obs': (1) length of the sequence of observations                 
//   'dim_z': (1) dimension of z (number of profiles).                   
//   'y': (n_obs) control profile.                                       
//   'z': (n_obs,dim_z) profiles.                                        
//   'a': (1) alias 'alpha', model parameter                             
//   'b': (1) alias 'beta', model parameter                              
//   'gamma': (dim_z,3) model parameter                                  
//   'pratio': (n_obs,2) emission probability ratio                      
//                                                                       
// RETURN:                                                               
//   'void'                                                              
//                                                                       
// SIDE EFFECTS:                                                         
//   Updte 'pratio' in place.                                            
{

   int i;
   int j;
   int k;
   int n = *n_obs;
   int r = *dim_z;
   // The following aliases are to avoid the statement '1/*beta'
   // which is interpreted as a comment.
   double alpha = *a;
   double beta = *b;

   // Compute p's.
   double p[(r+2)*3];
   for (i = 0 ; i < 3 ; i++) {
      double denom = 1 + 1/beta;
      for (j = 0 ; j < r ; j++) denom += gamma[j+i*r];
      for (j = 0 ; j < r ; j++) p[(j+2)+i*(r+2)] = gamma[j+i*r] / denom;
      // Fill in p_0 and p_1.
      p[0+i*(r+2)] = 1/beta / denom;
      p[1+i*(r+2)] = 1 / denom;
   }

   // Compute q's in log space.
   double logq[(r+2)*2];
   for (i = 0 ; i < 2 ; i++) {
   for (j = 0 ; j < r+2 ; j++) {
      logq[j+i*(r+2)] = log(p[j+i*(r+2)]) - log(p[j+2*(r+2)]);
   }
   }

   for (k = 0 ; k < n ; k++) {
      // NAs of type 'int' is a large negative value. Set ratio to
      // 1.0 in case of NA emission (assuming all states have the
      // same probability of producing NAs).
      int isna = 0;
      for (i = 0 ; i < r ; i++) {
         if (z[i+k*r] < 0) {
            isna = 1;
            break;
         }
      }
      if (isna || y[k] < 0) {
         pratio[2*k] = 1.0;
         pratio[2*k+1] = 1.0;
         continue;
      }
      // Compute log-ratio (no caching).
      double p1_p0 = alpha * logq[0] + y[k] * logq[1];
      double p2_p0 = alpha * logq[r+2] + y[k] * logq[r+3];
      for (i = 0 ; i < r ; i++) {
         p1_p0 += z[i+k*r] * logq[i+2];
         p2_p0 += z[i+k*r] * logq[i+2+(r+2)];
      }

      // Take exponential.
      pratio[2*k] = exp(p1_p0);
      pratio[2*k+1] = exp(p2_p0);

   }

   return;
}

void
fwdb (
   // input //
   int *nblocks,
   int *lengths,
   // params //
   double *Q,
   double *init,
   // output //
   double *pratio,
   double *phi,
   double *sumtrans
)
// SYNOPSIS:                                                             
//   Wrapper for the forward-backward routine, for an easy call from R.  
//                                                                       
// ARGUMENTS:                                                            
//   'nblocks': (1) number of blocks of the time series.                 
//   'lengths': (*nblocks) length of each block of the time series.      
//   'Q': (3,3) transition probabilities.
//   'init': (3) initial probability of the first state                  
//   'pratio': (2,n) ratio of emission probabilities                     
//   'phi': (2,n) smoothed probabilities for the first 2 states          
//   'sumtrans': (3,3) sum of conditional transitions probabilties       
//                                                                       
// RETURN:                                                               
//   'void'                                                              
//                                                                       
// SIDE EFFECTS:                                                         
//   Update 'sumtrans' and 'phi' in place, replace 'pratio' by the       
//   forward alpha probabilities.

{

   int i;
   int start = 0;

   for (i = 0 ; i < 3*3 ; i++) sumtrans[i] = 0.0;
   for (i = 0 ; i < *nblocks ; i++) {
      spcfwdb(lengths[i], Q, init, pratio+start, phi+start, sumtrans);
      start += 2*lengths[i];
   }

   return;
}
