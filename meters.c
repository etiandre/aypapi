#include "meters.h"
#include "aypapi.h"
#include <stdio.h>
#include <unistd.h>

void
print_meters_header(struct data* data)
{
  printf("Time [s]\t");
  for (int m = 0; m < (int)N_METERS; m++) {
    for (int u = 0; u < data->uncores; u++) {
      printf("%s %d\t", METER_NAMES[m], u);
    }
  }
  printf("\n");
}

/** prints interleaved meters */
void
print_meters(struct data* data, double t)
{
  printf(FLOAT_PRECISION_FMT "\t", t);
  for (int m = 0; m < (int)N_METERS; m++) {
    for (int u = 0; u < data->uncores; u++) {
      printf(FLOAT_PRECISION_FMT "\t", data->meters[m + u * N_METERS]);
    }
  }
  printf("\n");
}

double
get_meter(struct data* data, int uncore_idx, enum METERS meter)
{
  return data->meters[uncore_idx * N_METERS + meter];
}

void
set_meter(struct data* data, int uncore_idx, enum METERS meter, double value)
{
  data->meters[uncore_idx * N_METERS + meter] = value;
}
