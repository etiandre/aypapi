#include "aypapi.h"
#include "meters.h"
#include "util.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MIN_MFLOPS 1000
#define K_OVERHEAD 0.98
#define MAX_UFREQ 2.7
#define MIN_UFREQ 1.2
#define STEP_UFREQ 0.1

static enum phase { UNK, CPU, MEM } * phase;
enum action
{
  DO_NOTHING,
  RESET_UFREQ,
  INC_UFREQ,
  DEC_UFREQ
};
static double* maxmflops;
static double* ufreq;

static void
set_ufreq(int uncore_idx, double f)
{
  // TODO: replace this with native calls
  // because this is slow as heck
  char buf[256];
  sprintf(buf,
          "likwid-setFrequencies --umin %0.1f --umax %0.1f -c %d",
          f,
          f,
          uncore_idx);
  LOGP("%s\n", buf);
  ufreq[uncore_idx] = f;
  system(buf);
}

static void
reset_ufreq(int uncore_idx)
{
  char buf[256];
  ufreq[uncore_idx] = MAX_UFREQ;
  sprintf(buf, "likwid-setFrequencies -ureset -c %d", uncore_idx);
  LOGP("%s\n", buf);
  system(buf);
}
void
regulator_init(int n_uncores)
{
  maxmflops = calloc(n_uncores, sizeof(double));
  phase = calloc(n_uncores, sizeof(enum phase));
  ufreq = calloc(n_uncores, sizeof(double));
  for (int u = 0; u < n_uncores; u++) {
    reset_ufreq(u);
  }
}

void
regulator_destroy()
{
  free(maxmflops);
  free(phase);
  free(ufreq);
}

static enum action
decide_action(int uncore_id, double oi, double mflops)
{
  // don't do anything if there isn't enough work being produced
  if (mflops < MIN_MFLOPS) {
    LOGP("Nothing happens\n");
    phase[uncore_id] = UNK;
    return DO_NOTHING;
  }

  // phase detection
  // on phase change, reset uncore freq
  if (oi > 1 && phase[uncore_id] != CPU) {
    LOGP("Detected CPU-intensive phase (oi=%0.2f)\n", oi);
    phase[uncore_id] = CPU;
    return RESET_UFREQ;
  } else if (oi < 1 && phase[uncore_id] != MEM) {
    LOGP("Detected memory-intensive phase (oi=%0.2f)\n", oi);
    phase[uncore_id] = MEM;
    return RESET_UFREQ;
  }

  // get new maximum mflops
  // if within acceptable overhead, decrease uncore freq
  // else increase uncore freq
  if (phase[uncore_id] != UNK) {
    if (maxmflops[uncore_id] < mflops) {
      maxmflops[uncore_id] = mflops;
      return DO_NOTHING;
    } else if (mflops > (double)K_OVERHEAD * maxmflops[uncore_id]) {
      return DEC_UFREQ;
    } else {
      return INC_UFREQ;
    }
  } else {
    return DO_NOTHING;
  }
}

void
regulate(struct data* data)
{
  for (int u = 0; u < data->uncores; u++) {
    double oi = get_meter(data, u, METER_OP_INT);
    double mflops = get_meter(data, u, METER_FLOPS);
    enum action action = decide_action(u, oi, mflops);
    switch (action) {
      case RESET_UFREQ:
        reset_ufreq(u);
        break;
      case INC_UFREQ:
        LOGP("increase %d\n", u);
        if (ufreq[u] + STEP_UFREQ <= MAX_UFREQ)
          set_ufreq(u, ufreq[u] + STEP_UFREQ);
        break;
      case DEC_UFREQ:
        LOGP("decrease %d\n", u);
        if (ufreq[u] - STEP_UFREQ >= MIN_UFREQ)
          set_ufreq(u, ufreq[u] - STEP_UFREQ);
        break;
      case DO_NOTHING:
        break;
    }
  }
}
