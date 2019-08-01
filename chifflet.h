#pragma once
#include <stdlib.h>

// CPU events defined here
enum CPU_EVTS
{
  EVT_DOUBLE,
  EVT_DOUBLE_128,
  EVT_DOUBLE_256
};
static const char* CPU_EVT_NAMES[] = { "FP_ARITH:SCALAR_DOUBLE:cpu=%d",
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

static const char* UNCORE_EVT_NAMES[] = {
  "rapl::RAPL_ENERGY_PKG:cpu=%d",
  "rapl::RAPL_ENERGY_DRAM:cpu=%d",
  "bdx_unc_imc0::UNC_M_CAS_COUNT:RD:cpu=%d",
  "bdx_unc_imc0::UNC_M_CAS_COUNT:WR:cpu=%d",
  "bdx_unc_imc1::UNC_M_CAS_COUNT:RD:cpu=%d",
  "bdx_unc_imc1::UNC_M_CAS_COUNT:WR:cpu=%d",
  "bdx_unc_imc4::UNC_M_CAS_COUNT:RD:cpu=%d",
  "bdx_unc_imc4::UNC_M_CAS_COUNT:WR:cpu=%d",
  "bdx_unc_imc5::UNC_M_CAS_COUNT:RD:cpu=%d",
  "bdx_unc_imc5::UNC_M_CAS_COUNT:WR:cpu=%d"
};

static const size_t N_CPU_EVTS = sizeof(CPU_EVT_NAMES) / sizeof(char*);
static const size_t N_UNCORE_EVTS = sizeof(UNCORE_EVT_NAMES) / sizeof(char*);

void
calculate_meters(long long cpu_val[],
                 long long uncore_val[],
                 double dt,
                 double meters[],
                 int n_uncores,
                 int n_cpus_per_uncore);
