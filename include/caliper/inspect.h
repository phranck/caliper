#ifndef CALIPER_INSPECT_H_
#define CALIPER_INSPECT_H_

#include "caliper/checks.h"
#include "caliper/components.h"

namespace cal {

/**
 * Walks a finished screen and runs the six checks over everything on it.
 *
 * This is the half of the checks that needs the graphics library: it reads the
 * geometry the layout actually produced, rather than the geometry somebody
 * intended. The two differ exactly where it matters, because a layout computes
 * and a person estimates.
 *
 * It runs after a screen is built and before it is shown, so a screen that does
 * not hold is caught on the bench rather than on the wall.
 *
 * @param screen The screen to walk.
 * @param report Called once per finding, or nullptr to only count.
 * @returns How many findings there were.
 */
int Inspect(Screen& screen, Report report);

/**
 * Walks a screen and writes every finding to the log.
 *
 * @param screen The screen to walk.
 * @returns How many findings there were, so a caller can refuse to show it.
 */
int InspectAndLog(Screen& screen);

}  // namespace cal

#endif  // CALIPER_INSPECT_H_
