#include <papi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "util.h"

#define N_UNCORE_COUNTERS 10
#define N_CPU_COUNTERS 3
#define T_SLEEP 1.
#define N_METERS 4

void add_event(int EventSet, char* name, int cpu) {
  char buf[255];
  sprintf(buf, name, cpu);
  printf("Add %s\n", buf);
  int retval = PAPI_add_named_event(EventSet, buf);
  if (retval != PAPI_OK) {
    printf("Activating multiplex\n");
    retval = PAPI_set_multiplex(EventSet);
    PAPIFAIL(retval)
    retval = PAPI_add_named_event(EventSet, buf);
    PAPIFAIL(retval);
  }
}

int main(int argc, char** argv) {
	int EventSetCPU = PAPI_NULL;
	int EventSetUncore = PAPI_NULL;
  int retval;

  if (argc != 3) {
    printf("Usage: %s <cpu list> <uncore list>", argv[0]);
    exit(1);
  }
	/* Initialize PAPI */
	if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) {
    perror("unable to initialize PAPI");
    exit(1);
  }
  retval = PAPI_create_eventset(&EventSetCPU);
  PAPIFAILREASON(retval, "cannot create CPU eventset\n");

  retval = PAPI_create_eventset(&EventSetUncore);
  PAPIFAILREASON(retval, "cannot create Uncore eventset\n");

  retval = PAPI_multiplex_init();
  PAPIFAILREASON(retval,"cannot init multiplex");

  // parse cpu list
  int n_cpu_events = 0;
  int n_cpus = 0;
  char* ptr = strtok(argv[1], ",");
  while (ptr != NULL) {
    int cpu = atoi(ptr);
    add_event(EventSetCPU, "FP_ARITH:SCALAR_DOUBLE:cpu=%d", cpu);
    add_event(EventSetCPU, "FP_ARITH:128B_PACKED_DOUBLE:cpu=%d", cpu);
    add_event(EventSetCPU, "FP_ARITH:256B_PACKED_DOUBLE:cpu=%d", cpu);
    n_cpu_events += N_CPU_COUNTERS;
    n_cpus++;
    ptr = strtok(NULL, ",");
  }

  // parse uncore list
  int n_uncore_events = 0;
  int n_uncores = 0;
  ptr = strtok(argv[2], ",");
  while (ptr != NULL) {
    int uncore = atoi(ptr);
    add_event(EventSetUncore, "rapl::RAPL_ENERGY_PKG:cpu=%d", uncore);
    add_event(EventSetUncore, "rapl::RAPL_ENERGY_DRAM:cpu=%d", uncore);
    add_event(EventSetUncore, "bdx_unc_imc0::UNC_M_CAS_COUNT:RD:cpu=%d", uncore);
    add_event(EventSetUncore, "bdx_unc_imc0::UNC_M_CAS_COUNT:WR:cpu=%d", uncore);
    add_event(EventSetUncore, "bdx_unc_imc1::UNC_M_CAS_COUNT:RD:cpu=%d", uncore);
    add_event(EventSetUncore, "bdx_unc_imc1::UNC_M_CAS_COUNT:WR:cpu=%d", uncore);
    add_event(EventSetUncore, "bdx_unc_imc4::UNC_M_CAS_COUNT:RD:cpu=%d", uncore);
    add_event(EventSetUncore, "bdx_unc_imc4::UNC_M_CAS_COUNT:WR:cpu=%d", uncore);
    add_event(EventSetUncore, "bdx_unc_imc5::UNC_M_CAS_COUNT:RD:cpu=%d", uncore);
    add_event(EventSetUncore, "bdx_unc_imc5::UNC_M_CAS_COUNT:WR:cpu=%d", uncore);
    n_uncore_events += N_UNCORE_COUNTERS;
    n_uncores++;
    ptr = strtok(NULL, ",");
  }

  retval = PAPI_start(EventSetCPU);
  PAPIFAILREASON(retval, "Cannot start CPU meters");
  retval = PAPI_start(EventSetUncore);
  PAPIFAILREASON(retval, "Cannot start uncore meters");

  /* Daemon */
  long long cpu_val[n_cpu_events];
  long long uncore_val[n_uncore_events];
  double flop, datavolume;
  double *cpu_power, *dram_power;
  cpu_power = calloc(n_cpus, sizeof(double));
  dram_power = calloc(n_uncores, sizeof(double));
  while (1) {
    sleep(T_SLEEP);
    PAPI_accum(EventSetCPU, cpu_val);
    PAPI_accum(EventSetUncore, uncore_val);

    print_events(cpu_val, n_cpu_events);
    print_events(uncore_val, n_uncore_events);

    // process meters
    //TODO check *all* units !!!!
    // cpu!
    flop = 0;
    for (int i = 0; i < n_cpu_events; i+=N_CPU_COUNTERS)
      flop += cpu_val[i] * 2.0 + cpu_val[i+1] + cpu_val[i+2] * 4.0;

    // uncore!
    datavolume = 0;
    for (int i=0; i<n_uncore_events; i+=N_UNCORE_COUNTERS) {
      // TODO: do I need to zero cpu_power and dram_power ?
      cpu_power[i/N_UNCORE_COUNTERS] = uncore_val[i];
      dram_power[i/N_UNCORE_COUNTERS] = uncore_val[i+1];
      // TODO: check if it is the bon calcul
      datavolume += (uncore_val[i+2] + uncore_val[i+3]
                     + uncore_val[i+4] + uncore_val[i+5]
                     + uncore_val[i+6] + uncore_val[i+7]
                     + uncore_val[i+8] + uncore_val[i+9]
                     )*64.0;
    }
  }
  free(cpu_power);
  free(dram_power);
  return 0;
}
