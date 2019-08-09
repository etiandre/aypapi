#pragma once
#include "../aypapi.h"
#include <stdlib.h>

/**
 * An arch file should define all of these.
 * See broadwellEP.c in this directory for an example.
 */

/** Shorthands for cpu events */
enum CPU_EVTS;
/** Template names of cpu events. The %d will be replaced by the cpu index */
extern const char* CPU_EVT_NAMES[];
/** Number of cpu events */
extern const int N_CPU_EVTS;

/** Shorthands for uncore events */
enum UNCORE_EVTS;
/** Template names of uncore events. The %d will be replaced by the uncore index
 */
extern const char* UNCORE_EVT_NAMES[];
/** Number of uncore events */
extern const int N_UNCORE_EVTS;

/** Populates the meters values in data by calculating them from cpu_val and
 * uncore_val. This should be called after populating those but before calling
 * the regulator.
 * @param data pointer to the data struct
 * @param dt elapsed time between two readings, in seconds.
 */
void
calculate_meters(struct data* data, double dt);
