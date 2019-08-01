#pragma once
#include <papi.h>

#define AYPAPI_VERSION "git"
#define PAPIDIEREASON(r, s)                                                    \
  if (r != PAPI_OK) {                                                          \
    PAPI_perror(s);                                                            \
    exit(1);                                                                   \
  }
#define PAPIDIE(r) PAPIDIEREASON(r, "failing");

#define DIE(...)                                                               \
  {                                                                            \
    fprintf(stderr, __VA_ARGS__);                                              \
    exit(1);                                                                   \
  }
#define LOGP(...)                                                              \
  {                                                                            \
    fprintf(stderr, __VA_ARGS__);                                              \
  }
#define DEBUGP(...)                                                            \
  if (verbose) {                                                               \
    LOGP(__VA_ARGS__)                                                          \
  }

#define FLOAT_PRECISION_FMT "%0.2f"

void
print_events(long long values[], size_t size);
