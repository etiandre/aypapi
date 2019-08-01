#pragma once
#include "../aypapi.h"
#include <stdlib.h>
// CPU events defined here
enum CPU_EVTS;
extern const char* CPU_EVT_NAMES[];
extern const int N_CPU_EVTS;

enum UNCORE_EVTS;
extern const char* UNCORE_EVT_NAMES[];
extern const int N_UNCORE_EVTS;

void
calculate_meters(struct data* data, double dt);
