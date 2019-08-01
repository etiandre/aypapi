#pragma once
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

static const size_t N_METERS = sizeof(METER_NAMES) / sizeof(char*);

void
print_meters_header(int n_uncores);

void
print_meters(double meters[], double t, int n_uncores);
