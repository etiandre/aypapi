#pragma once
#include <argp.h>
#include <stdbool.h>

struct arguments {
  unsigned int sleep_usec;
  char* uncore_list;
  bool verbose;
};
struct argp argp;
