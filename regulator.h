#include "aypapi.h"

/** Inits the regulator and resets uncore frequency on all uncores.
 * @param n_uncores number of uncores that will be metered.
 */
void
regulator_init(int n_uncores);

/** Destroys the regulator and resets uncore frequency on all uncores */
void
regulator_destroy();

/** Regulates the uncores by reading meter_val from data.
    This needs to be called after calculating the meters.
*/
void
regulate(struct data* data);
