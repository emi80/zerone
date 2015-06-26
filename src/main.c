#include "predict.h"
#include <stdio.h>
#include "samread.h"
#include "zerone.h"

int main(int argc, char **argv) {

   // Read files and build ChIP structure.
   int any_sam = 0;
   int all_sam = 1;
   for (int i = 1; i < argc; i++) {
      if (is_sam(argv[i])) any_sam = 1;
      else all_sam = 0;
   }

   ChIP_t *ChIP = NULL;
   if (any_sam && !all_sam) {
      fprintf(stderr, "%s\n", "different file formats.");
      return 1;

   } else if (any_sam && all_sam) {
      char * samfiles[argc-1];
      unsigned int nfiles = 0;
      for (int i = 1; i < argc; i++) samfiles[nfiles++] = argv[i];
      ChIP = read_sam(samfiles, nfiles);

   } else if (argc == 2 && !any_sam) {
      FILE *inputf = fopen(argv[1], "r");
      if (inputf == NULL) {
         fprintf(stderr, "file not found: %s\n", argv[1]);
         return 1;
      }
      ChIP = read_file(inputf);
      fclose(inputf);
   }

   // Do zerone.
   const unsigned int m = 3; // number of states.
   zerone_t *zerone = do_zerone(ChIP);
   if (zerone == NULL) return 1;

   // Print results (Viretbi path and phi matrix).
   for (size_t i = 0 ; i < nobs(ChIP) ; i++) {
      fprintf(stdout, "%d\t%f\t%f\t%f\n", zerone->path[i],
            zerone->phi[0+i*m], zerone->phi[1+i*m], zerone->phi[2+i*m]);
   }

   // Quality control.
   char * centerfn = "/home/pcusco/Zerone/classifier/SVM_18x1_center.csv";
   char * scalefn  = "/home/pcusco/Zerone/classifier/SVM_18x1_scale.csv";
   char * svfn     = "/home/pcusco/Zerone/classifier/SVM_200x18_sv.csv";
   char * coefsfn  = "/home/pcusco/Zerone/classifier/SVM_200x1_coefs.csv";

   double * coefs  = readmatrix(coefsfn, NSV, 1);
   double * center = readmatrix(centerfn, DIM, 1);
   double * scale  = readmatrix(scalefn, DIM, 1);
   double * sv     = readmatrix(svfn, NSV, DIM);
   if (coefs == NULL || center == NULL || scale == NULL || sv == NULL) {
      fprintf(stderr, "could not read SVM file\n");
      return 1;
   }

   double * feat = extractfeats(ChIP, zerone);
   double * sfeat = zscale(feat, center, scale);

   char * advice = predict(sfeat, sv, coefs) >= 0 ? "accept" : "reject";
   fprintf(stderr, "advice: %s discretization.\n", advice);

//char * featsfn = "/home/pcusco/zerone/classifier/SVM_946x18_features.csv";
//char * labelsfn = "/home/pcusco/zerone/classifier/SVM_946x1_labels.csv";
//double * feats = readmatrix(featsfn, 946, DIM);
//double * labels = readmatrix(labelsfn, 946, 1);
//
//int sum = 0;
//for (int i = 0; i < 946; i++) {
//   double * sfeat = zscale(&feats[i*DIM], center, scale);
//   int p = predict(sfeat, sv, coefs);
//   free(sfeat);
//   if (p == -1) p = 0;
//   if (p != (int)labels[i]) {
//      sum++;
//   }
//      fprintf(stdout, "%d:%d\n", (int)labels[i], p);
//}
//fprintf(stderr, "sum: %d\n", sum);

   free(center);
   free(scale);
   free(sv);
   free(coefs);
   free(feat);
   free(sfeat);

   destroy_zerone_all(zerone); // Also frees ChIP.

   return 0;
}