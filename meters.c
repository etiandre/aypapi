#include "meters.h"
#include <stdio.h>
#include <unistd.h>

/** prints interleaved header */
void print_meters_header(int n_uncores) {
  for(int m=0; m<(int)N_METERS; m++) {
    for (int u=0; u<n_uncores; u++) {
      printf("%s %d\t", METER_NAMES[m], u);
    }
  }
    printf("\n");
}

/** prints interleaved meters */
void print_meters(double *meters, int n_uncores) {
  for(int m=0; m<(int)N_METERS; m++) {
    for (int u=0; u<n_uncores; u++) {
      printf("%0.2f\t", meters[m+u*N_METERS]);
    }
  }
  printf("\n");
}
