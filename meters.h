#pragma once
#include "aypapi.h"
#include <unistd.h>

enum METERS
{
  METER_FLOPS,
  METER_MEM_BW,
  METER_PKG_PWR,
  METER_DRAM_PWR,
  METER_OP_INT,
};

static const char* METER_NAMES[] = {
  "MFLOP/s",        "Memory Bandwidth [MB/s]", "Package Power [W]",
  "DRAM_Power [W]", "Operational Intensity",
};

static const int N_METERS = sizeof(METER_NAMES) / sizeof(char*);

void
print_meters_header(struct data* data);

void
print_meters(struct data* data, double t);

double
get_meter(struct data* data, int uncore_idx, enum METERS meter);

void
set_meter(struct data* data, int uncore_idx, enum METERS meter, double value);
