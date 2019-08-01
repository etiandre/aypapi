#include "util.h"
#include <stdio.h>
#include <unistd.h>

void
print_events(long long values[], size_t size)
{
  for (size_t i = 0; i < size; i++) {
    LOGP("Read (%ld) = %lld\n", i, values[i]);
  }
}
