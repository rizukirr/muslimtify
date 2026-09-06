#ifndef CHECK_CYCLE_H
#define CHECK_CYCLE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Fire a missed trigger only if it came due within this many minutes of now.
// Covers any realistic adhan length and short cycle overruns, while dropping
// prayers missed by a long suspend/resume rather than replaying them.
#define CATCHUP_MAX_MIN 15

/**
 * Decision for a single cache trigger given the current minute-of-day.
 * KEEP: trigger is still in the future — leave it in the cache.
 * FIRE: trigger is due now or was missed within the catch-up window — fire it.
 * DROP: trigger was missed longer ago than the catch-up window — remove it
 *       without firing (avoids replaying stale adhans after suspend/resume).
 */
typedef enum {
  TRIGGER_KEEP = 0,
  TRIGGER_FIRE,
  TRIGGER_DROP,
} TriggerAction;

/**
 * Classify a trigger by its scheduled minute-of-day vs. the current minute.
 * Pure function (no I/O) so it can be unit-tested. See TriggerAction.
 */
TriggerAction trigger_catchup_action(int trigger_minute, int current_minute);

/**
 * True when a cache dated cache_date is valid as-is for today. Deliberately
 * ignores trigger_count: a cache carrying today's date is valid even with no
 * triggers left, because an empty list means today's prayers are all done,
 * not that the cache is missing. Requiring a nonzero trigger count here would
 * force a rebuild every cycle after the last trigger fires, and with the
 * widened catch-up gate in cache_build_triggers that rebuild would readmit
 * the trigger that just fired and fire it again, once a minute, forever.
 * trigger_count is taken as a parameter, rather than dropped, so a test can
 * prove it has no effect on the result.
 * Pure function (no I/O) so it can be unit-tested.
 */
bool cache_is_valid_for_today(const char *cache_date, int trigger_count, const char *today);

/**
 * Run one prayer-notification check for the current minute.
 * Loads (or rebuilds) the prayer cache, fires any due adhan/reminder
 * notifications, and prunes elapsed triggers.
 * Returns: 0 on success, non-zero on error.
 */
int run_check_cycle(void);

#ifdef __cplusplus
}
#endif

#endif // CHECK_CYCLE_H
