#include <papi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include "util.h"
#include "meters.h"
#include "chifflet.h"

#define T_SLEEP 1

int debug = 0;

void add_event(int EventSet, const char* name, int cpu) {
  char buf[255];
  int code;
  int retval;
  PAPI_event_info_t info;

  sprintf(buf, name, cpu);
  retval = PAPI_event_name_to_code(buf, &code);
  PAPIFAILREASON(retval, "event_name_to_code");

  retval = PAPI_get_event_info(code, &info);
  PAPIFAILREASON(retval, "get_event_info");
  //TODO check 1/update_freq très inférieur à période du régulateur
  //     (c'est toujours = 0 sur les compteurs utilisés sur chifflet, ce qui est chelou)
  if (debug) printf("Adding %s (datatype=%d, freq=%d)\n", buf, info.data_type, info.update_freq);
  retval = PAPI_add_event(EventSet, code);
  if (retval != PAPI_OK) {
    if (debug) printf("Activating multiplex\n");
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
    printf("Usage: %s <cpu list> <uncore list>\n", argv[0]);
    exit(1);
  }

  if (getenv("AYPAPI_DEBUG")) {
    printf("Activating debug output\n");
    debug = 1;
  }

	/* Initialize PAPI */
	if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) {
    perror("unable to initialize PAPI");
    exit(1);
  }

  retval = PAPI_multiplex_init();
  PAPIFAILREASON(retval,"cannot init multiplex");

  // create the CPU eventset and set options
  retval = PAPI_create_eventset(&EventSetCPU);
  PAPIFAILREASON(retval, "cannot create CPU eventset\n");

  // tell PAPI that the events on this eventset will be using the CPU component (#0)
  // this is required for setting options
  // if we added events to this eventset before setting options, PAPI would already know that
  // the eventset would be using the CPU component, so this wouldn't be needed. But some options
  // can only be set to an empty eventset...
  retval = PAPI_assign_eventset_component(EventSetCPU, 0);
  PAPIFAILREASON(retval, "assign cpu eventset");

  // Set the domain as high as it will go, to measure absolutely everything that's happening on the machine
  // Not sure about what this exactly does, but i'm not taking any chances
  // this can only be done to an empty eventset
  // setting to ALL or another high value (eg KERNEL) requires root access
  PAPI_option_t options;
	options.domain.eventset = EventSetCPU;
	options.domain.domain = PAPI_DOM_ALL;
	retval = PAPI_set_opt( PAPI_DOMAIN, &options );
  PAPIFAILREASON(retval, "set opt PAPI_DOMAIN");

  // Set granularity to "current CPU". That is to say, core, or PU in hwloc terms.
  // By default PAPI only reads what's happening in the current thread (e.g. this program)
  // We want everything that's happening on the core in our case.
  // There is a PAPI_set_cmp_granularity function that can be called before creating
  // eventsets, but for some reason it prevents us from adding events belonging to different cpus
  // this works tho
	options.granularity.eventset = EventSetCPU;
	options.granularity.granularity = PAPI_GRN_SYS;
	retval = PAPI_set_opt( PAPI_GRANUL, &options );
  PAPIFAILREASON(retval, "set opt PAPI_GRANUL");

  // Create the Uncore event set and set options.
  // What we are measuring on these can't be broken down to the per-core level anyway
  // so not setting any granularity options should work.
  retval = PAPI_create_eventset(&EventSetUncore);
  PAPIFAILREASON(retval, "cannot create Uncore eventset\n");

  // parse cpu list
  int n_cpus = 0;
  char* ptr = strtok(argv[1], ",");
  while (ptr != NULL) {
    int cpu = atoi(ptr);
    for (size_t i=0; i<N_CPU_EVTS; i++)
      add_event(EventSetCPU, CPU_EVT_NAMES[i], cpu);
    n_cpus++;
    ptr = strtok(NULL, ",");
  }
  const int tot_cpu_evts = N_CPU_EVTS * n_cpus;

  // parse uncore list
  int n_uncores = 0;
  ptr = strtok(argv[2], ",");
  while (ptr != NULL) {
    int uncore = atoi(ptr);
    for (size_t i=0; i<N_UNCORE_EVTS; i++)
      add_event(EventSetUncore, UNCORE_EVT_NAMES[i], uncore);
    n_uncores++;
    ptr = strtok(NULL, ",");
  }
  const int tot_uncore_evts = N_UNCORE_EVTS * n_uncores;
  const int n_cpus_per_uncore = n_cpus / n_uncores;

  if (debug) {
    printf("N_CPU_EVTS=%ld, N_UNCORE_EVTS=%ld, N_METERS=%ld\n", N_CPU_EVTS, N_UNCORE_EVTS, N_METERS);
    printf("n_cpus=%d, n_uncores=%d\n", n_cpus, n_uncores);
    printf("tot_cpu_evts=%d, tot_uncore_evts=%d\n", tot_cpu_evts, tot_uncore_evts);
    printf("n_cpus_per_uncore=%d\n", n_cpus_per_uncore);
  }

  /* Daemon */
  long long cpu_val[tot_cpu_evts];
  long long uncore_val[tot_uncore_evts];
  double meters[N_METERS * n_uncores];
  long long t0, t1;
  double dt;
  // start the counters
  retval = PAPI_start(EventSetCPU);
  PAPIFAILREASON(retval, "Cannot start CPU meters");
  retval = PAPI_start(EventSetUncore);
  PAPIFAILREASON(retval, "Cannot start uncore meters");
  // start time measurment
  // the nanosecond clock can measure about 4 s at most (wraps around after that), which is fine for our purposes
  t0 = PAPI_get_real_nsec();
  print_meters_header(n_uncores);

  while (1) {
    //TODO: make this configurable by a command line switch
    //TODO: maybe use a more precise function ?
    //TODO: account for execution time of this program ?
    sleep(T_SLEEP);
    // reset arrays
    // not using memset because floats may not behave as expected
    for (int i=0; i<tot_cpu_evts; i++)
      cpu_val[i] = 0.0;
    for (int i=0; i<tot_uncore_evts; i++)
      uncore_val[i] = 0.0;
    // read counters and reset them
    PAPI_accum(EventSetCPU, cpu_val);
    PAPI_accum(EventSetUncore, uncore_val);
    // read time
    t1 = PAPI_get_real_nsec();
    dt = (t1 - t0) / 1000000000;
    t0 = t1;
    if (debug) {
      printf("t=%lld, dt = %f\n", t1, dt);
      print_events(cpu_val, tot_cpu_evts);
      print_events(uncore_val, tot_uncore_evts);
    }
    // calculate meters for each uncore
    calculate_meters(cpu_val, uncore_val, dt, meters, n_uncores, n_cpus_per_uncore);
    print_meters(meters, n_uncores);
  }
  return 0;
}
