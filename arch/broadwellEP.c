#include "../aypapi.h"
#include "../meters.h"
#include "event.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

enum CPU_EVTS
{
  EVT_DOUBLE,
  EVT_DOUBLE_128,
  EVT_DOUBLE_256
};
const char* CPU_EVT_NAMES[] = { "FP_ARITH:SCALAR_DOUBLE:cpu=%d",
                                "FP_ARITH:128B_PACKED_DOUBLE:cpu=%d",
                                "FP_ARITH:256B_PACKED_DOUBLE:cpu=%d" };

enum UNCORE_EVTS
{
  EVT_PKG_ENERGY,
  EVT_DRAM_ENERGY,
  EVT_CAS_RD_0,
  EVT_CAS_WR_0,
  EVT_CAS_RD_1,
  EVT_CAS_WR_1,
  EVT_CAS_RD_4,
  EVT_CAS_WR_4,
  EVT_CAS_RD_5,
  EVT_CAS_WR_5,
};

const char* UNCORE_EVT_NAMES[] = { "rapl::RAPL_ENERGY_PKG:cpu=%d",
                                   "rapl::RAPL_ENERGY_DRAM:cpu=%d",
                                   "bdx_unc_imc0::UNC_M_CAS_COUNT:RD:cpu=%d",
                                   "bdx_unc_imc0::UNC_M_CAS_COUNT:WR:cpu=%d",
                                   "bdx_unc_imc1::UNC_M_CAS_COUNT:RD:cpu=%d",
                                   "bdx_unc_imc1::UNC_M_CAS_COUNT:WR:cpu=%d",
                                   "bdx_unc_imc4::UNC_M_CAS_COUNT:RD:cpu=%d",
                                   "bdx_unc_imc4::UNC_M_CAS_COUNT:WR:cpu=%d",
                                   "bdx_unc_imc5::UNC_M_CAS_COUNT:RD:cpu=%d",
                                   "bdx_unc_imc5::UNC_M_CAS_COUNT:WR:cpu=%d" };

const int N_CPU_EVTS = sizeof(CPU_EVT_NAMES) / sizeof(char*);
const int N_UNCORE_EVTS = sizeof(UNCORE_EVT_NAMES) / sizeof(char*);

void
calculate_meters(struct data* data, double dt)
{
  double flop, datavolume;
  double cpu_nrj, dram_nrj;
  for (int u_idx = 0; u_idx < data->uncores; u_idx++) {
    int cpu_val_off = u_idx * N_CPU_EVTS;
    int uncore_val_off = u_idx * N_UNCORE_EVTS;
    int meters_off = u_idx * N_METERS;
    flop = 0;
    for (int i = 0; i < N_CPU_EVTS * data->cpus / data->uncores;
         i += N_CPU_EVTS) {
      flop += data->cpu_val[cpu_val_off + i + EVT_DOUBLE] * 2.0 +
              data->cpu_val[cpu_val_off + i + EVT_DOUBLE_128] +
              data->cpu_val[cpu_val_off + i + EVT_DOUBLE_256] * 4.0;
    }
    // cpu and dram energy, unit is 2^-32 Joules
    cpu_nrj = data->uncore_val[uncore_val_off + EVT_PKG_ENERGY];
    dram_nrj = data->uncore_val[uncore_val_off + EVT_DRAM_ENERGY];

    // number of read and write instructions
    // multiply by 64 (size of cach line), cf. somewhere in the intel docs
    datavolume = (data->uncore_val[uncore_val_off + EVT_CAS_RD_0] +
                  data->uncore_val[uncore_val_off + EVT_CAS_WR_0] +
                  data->uncore_val[uncore_val_off + EVT_CAS_RD_1] +
                  data->uncore_val[uncore_val_off + EVT_CAS_WR_1] +
                  data->uncore_val[uncore_val_off + EVT_CAS_RD_4] +
                  data->uncore_val[uncore_val_off + EVT_CAS_WR_4] +
                  data->uncore_val[uncore_val_off + EVT_CAS_RD_5] +
                  data->uncore_val[uncore_val_off + EVT_CAS_WR_5]) *
                 64;
    // populate meters array
    data->meters[meters_off + METER_FLOPS] =
      flop * pow(10, -6) / dt; // [MFLOP count] / [s]
    data->meters[meters_off + METER_MEM_BW] =
      datavolume * pow(10, -6) / dt; // [MB] / [s] (base 10)
    data->meters[meters_off + METER_PKG_PWR] =
      cpu_nrj * pow(2, -32) / dt; // [J] / [s] = [W]
    data->meters[meters_off + METER_DRAM_PWR] =
      dram_nrj * pow(2, -32) / dt;                               // same
    data->meters[meters_off + METER_OP_INT] = flop / datavolume; // [No unit]
  }
}
