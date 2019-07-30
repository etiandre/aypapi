#pragma once
#include <papi.h>

#define PAPIFAILREASON(r, s) if (r != PAPI_OK) {PAPI_perror(s); exit(1);}
#define PAPIFAIL(r) PAPIFAILREASON(r, "failing");

void print_events(long long values[], size_t size);

