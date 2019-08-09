#pragma once
#include "aypapi.h"
#include <unistd.h>

/** Defines shorthands for the different meters. */
enum METERS
{
  METER_FLOPS,
  METER_MEM_BW,
  METER_PKG_PWR,
  METER_DRAM_PWR,
  METER_OP_INT,
};
/** Defines the header names of these meters */
static const char* METER_NAMES[] = {
  "MFLOP/s",        "Memory Bandwidth [MB/s]", "Package Power [W]",
  "DRAM_Power [W]", "Operational Intensity",
};
/** Number of meters defined for one uncore. */
static const int N_METERS = sizeof(METER_NAMES) / sizeof(char*);

/** prints interleaved meters header */
void
print_meters_header(struct data* data);

/** prints interleaved meter data */
void
print_meters(struct data* data, double t);

/** get a meter value given an uncore index and a meter shorthand */
double
get_meter(struct data* data, int uncore_idx, enum METERS meter);

/** set a meter value given an uncore index and a meter shorthand */
void
set_meter(struct data* data, int uncore_idx, enum METERS meter, double value);
