#include "aypapi.h"
#include "arch/event.h"
#include "args.h"
#include "meters.h"
#include "regulator.h"
#include <argp.h>
#include <hwloc.h>
#include <papi.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/** global verbose flag. If true, DEBUGP() prints. */
int verbose = 0;

/** Prepares an event given its name template and a cpu index, and adds it to
 * the given EventSet. If it fails to add it, it tries again after activating
 * multiplex. If it fails again, the progam ends.
 * @param EventSet EventSet index.
 * @param name event name template, will be sprintfed with cpu
 * @param cpu index of the cpu
 */
static void
add_event(int EventSet, const char* name, int cpu)
{
  char buf[255];
  int code;
  int retval;
  PAPI_event_info_t info;

  sprintf(buf, name, cpu);
  retval = PAPI_event_name_to_code(buf, &code);
  PAPIDIEREASON(retval, "event_name_to_code");

  retval = PAPI_get_event_info(code, &info);
  PAPIDIEREASON(retval, "get_event_info");
  DEBUGP("Adding %s (datatype=%d, freq=%d)\n",
         buf,
         info.data_type,
         info.update_freq);
  retval = PAPI_add_event(EventSet, code);
  if (retval != PAPI_OK) {
    DEBUGP("Activating multiplex\n");
    retval = PAPI_set_multiplex(EventSet);
    PAPIDIE(retval);
    retval = PAPI_add_named_event(EventSet, buf);
    PAPIDIE(retval);
  }
}
/** Helper function to add the target arch's cpu events */
static void
add_cpu_events(int EventSet, int cpu)
{
  for (int i = 0; i < N_CPU_EVTS; i++) {
    add_event(EventSet, CPU_EVT_NAMES[i], cpu);
  }
}

/** Helper function to add the target arch's uncore events */
static void
add_uncore_events(int EventSet, int uncore)
{
  for (int i = 0; i < N_UNCORE_EVTS; i++) {
    add_event(EventSet, UNCORE_EVT_NAMES[i], uncore);
  }
}

static void
init_data(struct data* data, int cpus, int uncores)
{
  data->cpus = cpus;
  data->uncores = uncores;
  data->cpu_val = calloc(N_CPU_EVTS * cpus, sizeof(long long));
  data->uncore_val = calloc(N_UNCORE_EVTS * uncores, sizeof(long long));
  data->meters = calloc(N_METERS * uncores, sizeof(double));
}

static void
destroy_data(struct data* data)
{
  free(data->cpu_val);
  free(data->uncore_val);
  free(data->meters);
  free(data); // uuuuhhh
}

int
main(int argc, char** argv)
{
  int EventSetCPU = PAPI_NULL;
  int EventSetUncore = PAPI_NULL;
  int retval;
  struct arguments arguments;
  arguments.sleep_usec = 1000000; // 1s default
  arguments.verbose = false;
  arguments.uncore_list = "";
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // make verbose state global
  if (arguments.verbose)
    verbose = 1;

  /* Initialize PAPI */
  if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) {
    perror("unable to initialize PAPI");
    exit(1);
  }
  retval = PAPI_multiplex_init();
  PAPIDIEREASON(retval, "cannot init multiplex");

  // create the CPU eventset and set options
  retval = PAPI_create_eventset(&EventSetCPU);
  PAPIDIEREASON(retval, "cannot create CPU eventset\n");

  // tell PAPI that the events on this eventset will be using the CPU component
  // (#0). This is required for setting options. If we added events to this
  // eventset before setting options, PAPI would already know that the eventset
  // would be using the CPU component, so this wouldn't be needed. But some
  // options can only be set to an empty eventset...
  retval = PAPI_assign_eventset_component(EventSetCPU, 0);
  PAPIDIEREASON(retval, "assign cpu eventset");

  // Set the domain as high as it will go, to measure absolutely everything
  // that's happening on the machine. Not sure about what this exactly does, but
  // i'm not taking any chances. This can only be done to an empty eventset
  // Setting to ALL or another high value (eg KERNEL) requires root access
  PAPI_option_t options;
  options.domain.eventset = EventSetCPU;
  options.domain.domain = PAPI_DOM_ALL;
  retval = PAPI_set_opt(PAPI_DOMAIN, &options);
  PAPIDIEREASON(retval, "set opt PAPI_DOMAIN");

  // Set granularity to "current CPU". That is to say, core, (PU in hwloc
  // terms). By default PAPI only reads what's happening in the current thread
  // (this program). We want everything that's happening on the core in our
  // case. There is also a PAPI_set_cmp_granularity function that can be called
  // before creating eventsets, but for some reason it prevents us from adding
  // events belonging to different cpus. This works tho
  options.granularity.eventset = EventSetCPU;
  options.granularity.granularity = PAPI_GRN_SYS;
  retval = PAPI_set_opt(PAPI_GRANUL, &options);
  PAPIDIEREASON(retval, "set opt PAPI_GRANUL");

  // Create the Uncore event set and set options.
  // What we are measuring on these are physically happening on the whole
  // socket, so domain or granularity options are not needed.
  retval = PAPI_create_eventset(&EventSetUncore);
  PAPIDIEREASON(retval, "cannot create Uncore eventset\n");

  int cpus = 0;
  int uncores = 0;
  // parse uncore list and determine on which cores we're going to meter stuff
  // TODO error handling (bad input)
  if (!strcmp(arguments.uncore_list, "")) {
    // TODO if no arguments are given, the program should detect and enable all
    // availiable uncores.
    DIE("Socket detection not implemented, please use -s\n");
  }
  hwloc_topology_t topology;
  if (hwloc_topology_init(&topology) == -1)
    DIE("Error while initializing hwloc topology module\n");
  if (hwloc_topology_load(topology) == -1)
    DIE("Error while loading topology\n");
  char* ptr = strtok(arguments.uncore_list, ",");
  while (ptr != NULL) {
    int uncore = atoi(ptr);
    add_uncore_events(EventSetUncore, uncore);
    hwloc_obj_t cpu_obj = hwloc_get_obj_below_by_type(
      topology, HWLOC_OBJ_PACKAGE, uncore, HWLOC_OBJ_PU, 0);
    if (cpu_obj == NULL)
      DIE("No cores found on socket %d, aborting.\n", uncore);
    int idx = 0;
    DEBUGP("Socket %d: metering on PUs no ", uncore);
    while (cpu_obj) {
      DEBUGP("%d ", cpu_obj->os_index);
      add_cpu_events(EventSetCPU, (int)cpu_obj->os_index);
      cpus++;
      idx++;
      cpu_obj = hwloc_get_obj_below_by_type(
        topology, HWLOC_OBJ_PACKAGE, uncore, HWLOC_OBJ_PU, idx);
    }
    DEBUGP("\n");
    uncores++;
    ptr = strtok(NULL, ",");
  }

  /* Daemon */
  struct data data;
  init_data(&data, cpus, uncores);
  regulator_init(data.uncores);
  long long t0, t1, t;
  double dt;

  LOGP("aypapi version " AYPAPI_VERSION " starting\n");
  LOGP("Metering on %d sockets (%d PUs per socket) every %d µs\n",
       data.uncores,
       data.cpus / data.uncores,
       arguments.sleep_usec);
  // start the counters
  retval = PAPI_start(EventSetCPU);
  PAPIDIEREASON(retval, "Cannot start CPU meters");
  retval = PAPI_start(EventSetUncore);
  PAPIDIEREASON(retval, "Cannot start uncore meters");
  // start time measurment
  t = t0 = PAPI_get_real_usec();
  print_meters_header(&data);
  // TODO trap Ctrl-C instead of just dying
  while (1) {
    usleep(arguments.sleep_usec);
    // reset arrays
    for (int i = 0; i < N_CPU_EVTS * data.cpus; i++)
      data.cpu_val[i] = 0.0;
    for (int i = 0; i < N_UNCORE_EVTS * data.uncores; i++)
      data.uncore_val[i] = 0.0;
    // read counters and reset them
    PAPI_accum(EventSetCPU, data.cpu_val);
    PAPI_accum(EventSetUncore, data.uncore_val);
    // read time
    t1 = PAPI_get_real_usec();
    dt = (double)(t1 - t) / 1000000.0;
    t = t1;
    DEBUGP("t=%lld, dt = %f\n", t1, dt);
    // calculate meters for each uncore
    calculate_meters(&data, dt);
    print_meters(&data, (double)(t - t0) / 1000000.0);
    // call regulator
    regulate(&data);
  }
  // this is never called, see TODO above.
  destroy_data(&data);
  regulator_destroy();
  return 0;
}
