#include "args.h"
#include "aypapi.h"
#include <argp.h>
#include <stdlib.h>

const char* argp_program_version = "aypapi version " AYPAPI_VERSION;
const char* argp_program_bug_address = "http://github.com/etiandre/aypapi";
static struct argp_option options[] = {
  { "sleeptime",
    't',
    "time",
    0,
    "Time in seconds between two measurments.",
    0 },
  { "sockets",
    's',
    "uncore_list",
    0,
    "Comma-separated list of sockets / package indexes to meter.",
    0 },
  { "verbose", 'v', 0, 0, "Enable debug output.", 0 },
  { 0 }
};

static error_t
parse_opt(int key, char* arg, struct argp_state* state)
{
  struct arguments* arguments = state->input;
  switch (key) {
    case 't': {
      double x = atof(arg);
      if (x <= 0.0)
        DIE("Invalid value for -t\n");
      arguments->sleep_usec = x * 1000000.0;
      break;
    }
    case 's': {
      arguments->uncore_list = arg;
      break;
    }
    case 'v': {
      arguments->verbose = true;
      break;
    }
    case ARGP_KEY_ARG:
      return 0;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

struct argp argp = { options, parse_opt, 0, 0, 0, 0, 0 };
