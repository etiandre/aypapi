#include "meters.h"
#include "util.h"
#include <stdio.h>
#include <unistd.h>

/** prints interleaved header */
void
print_meters_header(int n_uncores)
{
  printf("Time [s]\t");
  for (int m = 0; m < (int)N_METERS; m++) {
    for (int u = 0; u < n_uncores; u++) {
      printf("%s %d\t", METER_NAMES[m], u);
    }
  }
  printf("\n");
}

/** prints interleaved meters */
void
print_meters(double* meters, double t, int n_uncores)
{
  printf(FLOAT_PRECISION_FMT "\t", t);
  for (int m = 0; m < (int)N_METERS; m++) {
    for (int u = 0; u < n_uncores; u++) {
      printf(FLOAT_PRECISION_FMT "\t", meters[m + u * N_METERS]);
    }
  }
  printf("\n");
}
