#include <unistd.h>
#include <stdio.h>

void print_events(long long values[], size_t size) {
  for (size_t i = 0; i < size; i++) {
    printf("Read (%ld) = %lld\n", i, values[i]);
  }
}
