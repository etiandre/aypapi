#pragma once
#include <papi.h>

/** Program version, printed in help and usage message */
#define AYPAPI_VERSION "git"

/** Dies on papi error after printing the error message and a custom message.
 * @param r PAPI api return value
 * @param s message string
 */
#define PAPIDIEREASON(r, s)                                                    \
  if (r != PAPI_OK) {                                                          \
    PAPI_perror(s);                                                            \
    exit(1);                                                                   \
  }
/** Dies on papi error after printing the error message.
 * @param r PAPI api return value
 */
#define PAPIDIE(r) PAPIDIEREASON(r, "failing");

/** Fail-and-exit macro.
 */
#define DIE(...)                                                               \
  {                                                                            \
    fprintf(stderr, __VA_ARGS__);                                              \
    exit(1);                                                                   \
  }
/** stderr printing macro. */
#define LOGP(...)                                                              \
  {                                                                            \
    fprintf(stderr, __VA_ARGS__);                                              \
  }
/** stderr printing macro, only prints if verbose is true. */
#define DEBUGP(...)                                                            \
  if (verbose) {                                                               \
    LOGP(__VA_ARGS__)                                                          \
  }
/** Defines the format for printing float values (e.g. meters) **/
#define FLOAT_PRECISION_FMT "%0.2f"

/**
 * This structure contains info about the cpus and sockets and stores numerical
 * results of readings. It is populated using init_data, and destroyed using
 * destroy_data (defined in aypapi.c).
 */
struct data
{
  int cpus;    /**< number of cpus in this machine */
  int uncores; /**< number of uncore / sockets in this machine */

  long long* cpu_val;    /**< cpu readings, in order, grouped by cpu. size =
                            N_CPU_EVTS * cpus */
  long long* uncore_val; /**< uncore readings, in order, grouped by uncore. size
                            = N_UNCORE_EVTS * uncores */
  double* meters; /**< values of meters calculated from cpu and uncore readings,
                     in order, grouped by uncore. size = N_METERS * uncores */
};
