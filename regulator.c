#include "aypapi.h"
#include "meters.h"
#include <likwid.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/** Minimum MFLOPs threshold for activation */
#define MIN_MFLOPS 1000
/** If current mflops is over K_OVERHEAD*maxmflops, consider we are within an
 * acceptable performance margin */
#define K_OVERHEAD 0.98
// TODO define these in arch-specific files, or better yet, detect it in
// initialization (not always possible)
/** Maximum uncore frequency */
#define MAX_UFREQ 2700
/** Minimum uncore frequency */
#define MIN_UFREQ 1200
/** Step when modifying uncore frequency */
#define STEP_UFREQ 100

extern int verbose;

enum action
{
  DO_NOTHING,
  RESET_UFREQ,
  INC_UFREQ,
  DEC_UFREQ
};
/** These arrays contain the state of each uncore */
/** Detected phase */
static enum phase { UNK, CPU, MEM } * phase;
/** Measured maximum mflops */
static double* maxmflops;
/* Current uncore frequency */
static int* ufreq;

/** Sets the uncore frequency by constraining it. Updates the ufreq array too.
 */
static void
set_ufreq(int uncore_idx, int f)
{
  DEBUGP("set uncore freq %d\n", f);
  int r;
  r = freq_setUncoreFreqMin(uncore_idx, f);
  if (r != 0)
    DIE("Error setting min uncore freq\n");
  r = freq_setUncoreFreqMax(uncore_idx, f);
  if (r != 0)
    DIE("Error setting max uncore freq\n");
  ufreq[uncore_idx] = f;
}

/** Resets the uncore frequency by un-constraining it. Updates the ufreq array
   too. in ufreq, set it to MAX_UFREQ, because for our purposes it's the same
   thing. */
static void
reset_ufreq(int uncore_idx)
{
  DEBUGP("reset ufreq\n");
  uint64_t minf = freq_getUncoreFreqMin(uncore_idx);
  uint64_t maxf = freq_getUncoreFreqMax(uncore_idx);
  if (minf == 0 || maxf == 0)
    DIE("Error reading uncore freq bounds\n");
  int r = freq_setUncoreFreqMin(uncore_idx, minf);
  if (r != 0)
    DIE("Error setting min uncore freq\n");
  r = freq_setUncoreFreqMax(uncore_idx, maxf);
  if (r != 0)
    DIE("Error setting max uncore freq\n");
  ufreq[uncore_idx] = 2700;
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
regulator_destroy(int n_uncores)
{
  for (int u = 0; u < n_uncores; u++) {
    reset_ufreq(u);
  }
  free(maxmflops);
  free(phase);
  free(ufreq);
}

/** Decides for an action to take given an uncore id, the operational intensity
 * and the current mflops measured on that uncore. */
static enum action
decide_action(int uncore_id, double oi, double mflops)
{
  // don't do anything if there isn't enough work being produced
  if (mflops < MIN_MFLOPS) {
    DEBUGP("Nothing happens: ");
    phase[uncore_id] = UNK;
    return DO_NOTHING;
  }

  // phase detection
  // on phase change, reset uncore freq
  if (oi > 1 && phase[uncore_id] != CPU) {
    DEBUGP("Detected CPU-intensive phase (oi=%0.2f) ", oi);
    phase[uncore_id] = CPU;
    return RESET_UFREQ;
  } else if (oi < 1 && phase[uncore_id] != MEM) {
    DEBUGP("Detected memory-intensive phase (oi=%0.2f) ", oi);
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

/** Iterates on all the uncores, decides for an action to take, and does it,
 * while logging a few things. */
void
regulate(struct data* data)
{
  for (int u = 0; u < data->uncores; u++) {
    DEBUGP("Socket #%d: ", u);
    double oi = get_meter(data, u, METER_OP_INT);
    double mflops = get_meter(data, u, METER_FLOPS);
    enum action action = decide_action(u, oi, mflops);
    switch (action) {
      case RESET_UFREQ:
        reset_ufreq(u);
        break;
      case INC_UFREQ:
        if (ufreq[u] + STEP_UFREQ <= MAX_UFREQ) {
          DEBUGP("increase: ");
          set_ufreq(u, ufreq[u] + STEP_UFREQ);
        } else
          DEBUGP("Cannot increase: already at max\n")
        break;
      case DEC_UFREQ:
        if (ufreq[u] - STEP_UFREQ >= MIN_UFREQ) {
          DEBUGP("decrease: ")
          set_ufreq(u, ufreq[u] - STEP_UFREQ);
        } else
          DEBUGP("Cannot dercease: already at min\n")
        break;
      case DO_NOTHING:
        DEBUGP("Do nothing !\n")
        break;
    }
  }
}
