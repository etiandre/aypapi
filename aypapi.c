#include <papi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <hwloc.h>
#include <argp.h>
#include "args.h"
#include "util.h"
#include "meters.h"
#include "chifflet.h"

int verbose = 0;

void add_event(int EventSet, const char* name, int cpu) {
  char buf[255];
  int code;
  int retval;
  PAPI_event_info_t info;

  sprintf(buf, name, cpu);
  retval = PAPI_event_name_to_code(buf, &code);
  PAPIDIEREASON(retval, "event_name_to_code");

  retval = PAPI_get_event_info(code, &info);
  PAPIDIEREASON(retval, "get_event_info");
  //TODO check 1/update_freq très inférieur à période du régulateur
  //     (c'est toujours = 0 sur les compteurs utilisés sur chifflet, ce qui est chelou)
  DEBUGP("Adding %s (datatype=%d, freq=%d)\n", buf, info.data_type, info.update_freq);
  retval = PAPI_add_event(EventSet, code);
  if (retval != PAPI_OK) {
    DEBUGP("Activating multiplex\n");
    retval = PAPI_set_multiplex(EventSet);
    PAPIDIE(retval);
    retval = PAPI_add_named_event(EventSet, buf);
    PAPIDIE(retval);
  }
}
void add_cpu_events(int EventSet, int cpu) {
  for (size_t i=0; i<N_CPU_EVTS; i++) {
    add_event(EventSet, CPU_EVT_NAMES[i], cpu);
  }
}

void add_uncore_events(int EventSet, int uncore) {
  for (size_t i=0; i<N_UNCORE_EVTS; i++) {
    add_event(EventSet, UNCORE_EVT_NAMES[i], uncore);
  }
}

int main(int argc, char** argv) {
	int EventSetCPU = PAPI_NULL;
	int EventSetUncore = PAPI_NULL;
  int retval;

  struct arguments arguments;
  arguments.sleep_usec = 1000000;
  arguments.verbose = false;
  arguments.uncore_list = "";
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // make verbose state global
  if (arguments.verbose) verbose = 1;
	/* Initialize PAPI */
	if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) {
    perror("unable to initialize PAPI");
    exit(1);
  }

  retval = PAPI_multiplex_init();
  PAPIDIEREASON(retval,"cannot init multiplex");

  // create the CPU eventset and set options
  retval = PAPI_create_eventset(&EventSetCPU);
  PAPIDIEREASON(retval, "cannot create CPU eventset\n");

  // tell PAPI that the events on this eventset will be using the CPU component (#0)
  // this is required for setting options
  // if we added events to this eventset before setting options, PAPI would already know that
  // the eventset would be using the CPU component, so this wouldn't be needed. But some options
  // can only be set to an empty eventset...
  retval = PAPI_assign_eventset_component(EventSetCPU, 0);
  PAPIDIEREASON(retval, "assign cpu eventset");

  // Set the domain as high as it will go, to measure absolutely everything that's happening on the machine
  // Not sure about what this exactly does, but i'm not taking any chances
  // this can only be done to an empty eventset
  // setting to ALL or another high value (eg KERNEL) requires root access
  PAPI_option_t options;
	options.domain.eventset = EventSetCPU;
	options.domain.domain = PAPI_DOM_ALL;
	retval = PAPI_set_opt( PAPI_DOMAIN, &options );
  PAPIDIEREASON(retval, "set opt PAPI_DOMAIN");

  // Set granularity to "current CPU". That is to say, core, or PU in hwloc terms.
  // By default PAPI only reads what's happening in the current thread (e.g. this program)
  // We want everything that's happening on the core in our case.
  // There is a PAPI_set_cmp_granularity function that can be called before creating
  // eventsets, but for some reason it prevents us from adding events belonging to different cpus
  // this works tho
	options.granularity.eventset = EventSetCPU;
	options.granularity.granularity = PAPI_GRN_SYS;
	retval = PAPI_set_opt( PAPI_GRANUL, &options );
  PAPIDIEREASON(retval, "set opt PAPI_GRANUL");

  // Create the Uncore event set and set options.
  // What we are measuring on these can't be broken down to the per-core level anyway
  // so not setting any granularity options should work.
  retval = PAPI_create_eventset(&EventSetUncore);
  PAPIDIEREASON(retval, "cannot create Uncore eventset\n");

  // parse uncore list and determine on which cores we're going to meter stuff
  // also set a few constants that help with array walking later
  if (!strcmp(arguments.uncore_list, "")) DIE("Socket detection not yet implemented, please use -s\n");
  int n_cpus = 0;
  int n_uncores = 0;
  hwloc_topology_t topology;
  if (hwloc_topology_init(&topology) == -1)
    DIE("Error while initializing hwloc topology module\n");
  if (hwloc_topology_load(topology) == -1)
    DIE("Error while loading topology\n");
  char* ptr = strtok(arguments.uncore_list, ",");
  while (ptr != NULL) {
    int uncore = atoi(ptr);
    add_uncore_events(EventSetUncore, uncore);
    hwloc_obj_t cpu_obj = hwloc_get_obj_below_by_type(topology, HWLOC_OBJ_PACKAGE, uncore, HWLOC_OBJ_PU, 0);
    if (cpu_obj == NULL)
      DIE("No cores found on socket %d, aborting.\n", uncore);
    int idx=0;
    DEBUGP("Socket %d: metering on PUs no ", uncore);
    while (cpu_obj) {
      DEBUGP("%d ", cpu_obj->os_index);
      add_cpu_events(EventSetCPU, (int)cpu_obj->os_index);
      n_cpus++;
      idx++;
      cpu_obj = hwloc_get_obj_below_by_type(topology, HWLOC_OBJ_PACKAGE, uncore, HWLOC_OBJ_PU, idx);
    }
    DEBUGP("\n");
    n_uncores++;
    ptr = strtok(NULL, ",");
  }
  const int tot_uncore_evts = N_UNCORE_EVTS * n_uncores;
  const int tot_cpu_evts = N_CPU_EVTS * n_cpus;
  const int n_cpus_per_uncore = n_cpus / n_uncores;

  if (verbose) {
    printf("N_CPU_EVTS=%ld, N_UNCORE_EVTS=%ld, N_METERS=%ld\n", N_CPU_EVTS, N_UNCORE_EVTS, N_METERS);
    printf("n_cpus=%d, n_uncores=%d\n", n_cpus, n_uncores);
    printf("tot_cpu_evts=%d, tot_uncore_evts=%d\n", tot_cpu_evts, tot_uncore_evts);
    printf("n_cpus_per_uncore=%d\n", n_cpus_per_uncore);
  }

  /* Daemon */
  LOGP("aypapi version " AYPAPI_VERSION " starting\n");
  LOGP("Metering on %d sockets (%d PUs per socket) every %d µs\n", n_uncores, n_cpus_per_uncore, arguments.sleep_usec);
  long long cpu_val[tot_cpu_evts];
  long long uncore_val[tot_uncore_evts];
  double meters[N_METERS * n_uncores];
  long long t0, t1, t;
  double dt;
  // start the counters
  retval = PAPI_start(EventSetCPU);
  PAPIDIEREASON(retval, "Cannot start CPU meters");
  retval = PAPI_start(EventSetUncore);
  PAPIDIEREASON(retval, "Cannot start uncore meters");
  // start time measurment
  // the nanosecond clock can measure about 4 s at most (wraps around after that), which is fine for our purposes
  t = t0 = PAPI_get_real_nsec();
  print_meters_header(n_uncores);
  while (1) {
    //TODO: make this configurable by a command line switch
    //TODO: account for execution time of this program ?
    usleep(arguments.sleep_usec);
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
    dt = (double)(t1 - t) / 1000000000.0;
    t = t1;
    if (verbose) {
      LOGP("t=%lld, dt = %f\n", t1, dt);
      print_events(cpu_val, tot_cpu_evts);
      print_events(uncore_val, tot_uncore_evts);
    }
    // calculate meters for each uncore
    calculate_meters(cpu_val, uncore_val, dt, meters, n_uncores, n_cpus_per_uncore);
    print_meters(meters, (double)(t-t0) / 1000000000.0, n_uncores);
  }
  return 0;
}
