#include "chifflet.h"
#include "meters.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
extern int debug; // debug flag defined in aypapi.c

// TODO: maybe there is a better way to handle these arrays
// than using offsets.
// maybe structs ?
void
calculate_meters(long long cpu_val[],
                 long long uncore_val[],
                 double dt,
                 double meters[],
                 int n_uncores,
                 int n_cpus_per_uncore)
{
  double flop, datavolume;
  double cpu_nrj, dram_nrj;
  for (int u_idx = 0; u_idx < n_uncores; u_idx++) {
    int cpu_val_off = u_idx * N_CPU_EVTS;
    int uncore_val_off = u_idx * N_UNCORE_EVTS;
    int meters_off = u_idx * N_METERS;
    flop = 0;
    for (int i = 0; i < (int)N_CPU_EVTS * n_cpus_per_uncore; i += N_CPU_EVTS) {
      flop += cpu_val[cpu_val_off + i + EVT_DOUBLE] * 2.0 +
              cpu_val[cpu_val_off + i + EVT_DOUBLE_128] +
              cpu_val[cpu_val_off + i + EVT_DOUBLE_256] * 4.0;
    }
    // cpu and dram energy, unit is 2^-32 Joules
    cpu_nrj = uncore_val[uncore_val_off + EVT_PKG_ENERGY];
    dram_nrj = uncore_val[uncore_val_off + EVT_DRAM_ENERGY];

    // number of read and write instructions
    // multiply by 64 (size of cach line), cf. somewhere in the intel docs
    datavolume = (uncore_val[uncore_val_off + EVT_CAS_RD_0] +
                  uncore_val[uncore_val_off + EVT_CAS_WR_0] +
                  uncore_val[uncore_val_off + EVT_CAS_RD_1] +
                  uncore_val[uncore_val_off + EVT_CAS_WR_1] +
                  uncore_val[uncore_val_off + EVT_CAS_RD_4] +
                  uncore_val[uncore_val_off + EVT_CAS_WR_4] +
                  uncore_val[uncore_val_off + EVT_CAS_RD_5] +
                  uncore_val[uncore_val_off + EVT_CAS_WR_5]) *
                 64;
    // populate meters array
    meters[meters_off + METER_FLOPS] =
      flop * pow(10, -6) / dt; // [MFLOP count] / [s]
    meters[meters_off + METER_MEM_BW] =
      datavolume * pow(10, -6) / dt; // [MB] / [s] (base 10)
    meters[meters_off + METER_PKG_PWR] =
      cpu_nrj * pow(2, -32) / dt; // [J] / [s] = [W]
    meters[meters_off + METER_DRAM_PWR] = dram_nrj * pow(2, -32) / dt; // same
    meters[meters_off + METER_OP_INT] = flop / datavolume; // [No unit]
  }
}
