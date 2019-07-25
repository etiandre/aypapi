#include <stdlib.h>

// CPU events defined here
enum CPU_EVTS {DOUBLE, DOUBLE_128, DOUBLE_256};
const char* CPU_EVT_NAMES[] = {
                               "FP_ARITH:SCALAR_DOUBLE:cpu=%d",
                               "FP_ARITH:128B_PACKED_DOUBLE:cpu=%d",
                               "FP_ARITH:256B_PACKED_DOUBLE:cpu=%d"
};

// Uncore / socket events defined here
enum UNCORE_EVTS {PKG_ENERGY, DRAM_ENERGY,
                  CAS_RD_0, CAS_WR_0,
                  CAS_RD_1, CAS_WR_1,
                  CAS_RD_4, CAS_WR_4,
                  CAS_RD_5, CAS_WR_5,
};
const char* UNCORE_EVT_NAMES[] = {
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


const size_t N_UNCORE_EVENTS = sizeof(UNCORE_EVT_NAMES) / sizeof(char*);
const size_t N_CPU_EVENTS = sizeof(CPU_EVT_NAMES) / sizeof(char*);
